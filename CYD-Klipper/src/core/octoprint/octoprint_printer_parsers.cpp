#include "../printer_integration.hpp"
#include "octoprint_printer_integration.hpp"
#include <ArduinoJson.h>

PrinterState OctoPrinter::parse_printer_state(JsonDocument& in)
{
    JsonObject flags = in["state"]["flags"];
    bool cancelling = flags["cancelling"];
    bool closedOrError = flags["closedOrError"];
    bool error = flags["error"];
    bool finishing = flags["finishing"];
    bool operational = flags["operational"];
    bool paused = flags["paused"];
    bool pausing = flags["pausing"];
    bool printing = flags["printing"];
    bool ready = flags["ready"];
    bool resuming = flags["resuming"];
    bool sdReady = flags["sdReady"];

    if (printing || resuming)
    {
        return PrinterState::PrinterStatePrinting;
    }
    else if (pausing || paused)
    {
        return PrinterState::PrinterStatePaused;
    }
    else if (cancelling || finishing || ready)
    {
        return PrinterState::PrinterStateIdle;
    }

    return PrinterState::PrinterStateError;
}

void OctoPrinter::parse_printer_status(JsonDocument& in)
{
    const char* text = in["state"]["text"];
    printer_data.state = parse_printer_state(in);

    if (printer_data.state == PrinterState::PrinterStateError)
    {
        if (text != NULL && (printer_data.state_message == NULL || strcmp(printer_data.state_message, text)))
        {
            printer_data.state_message = (char *)malloc(strlen(text) + 1);
            strcpy(printer_data.state_message, text);
        }
    }

    JsonObject temperature = in["temperature"];

    if (temperature.containsKey("bed"))
    {
        printer_data.temperatures[PrinterTemperatureDeviceIndexBed] = temperature["bed"]["actual"];
        printer_data.target_temperatures[PrinterTemperatureDeviceIndexBed] = temperature["bed"]["target"];
    }

    if (temperature.containsKey("tool0"))
    {
        printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] = temperature["tool0"]["actual"];
        printer_data.target_temperatures[PrinterTemperatureDeviceIndexNozzle1] = temperature["tool0"]["target"];
    }

    printer_data.can_extrude = printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] >= MIN_EXTRUDER_EXTRUDE_TEMP;
    printer_data.homed_axis = true;
}

void OctoPrinter::parse_job_state(JsonDocument& in)
{
    JsonObject job = in["job"];

    if (job.containsKey("file"))
    {
        const char* name = job["file"]["name"];

        if (name != NULL && (printer_data.print_filename == NULL || strcmp(printer_data.print_filename, name)))
        {
            printer_data.print_filename = (char *)malloc(strlen(name) + 1);
            strcpy(printer_data.print_filename, name);
        }
    }

    if (job.containsKey("filament") && job["filament"].as<JsonObject>() != NULL && job["filament"].containsKey("tool0"))
    {
        printer_data.filament_used_mm = job["filament"]["tool0"]["length"];
    }

    JsonObject progress = in["progress"];
    float completion = progress["completion"];
    printer_data.print_progress = completion / 100;
    printer_data.elapsed_time_s = progress["printTime"];
    printer_data.printed_time_s = progress["printTime"];
    printer_data.remaining_time_s = progress["printTimeLeft"];
}

float OctoPrinter::parse_job_state_progress(JsonDocument& in)
{
    float completion = in["progress"]["completion"];
    return completion / 100;
}

void OctoPrinter::parse_error(JsonDocument& in)
{
    const char* error = in["error"];
    if (error != NULL)
    {
        printer_data.state = PrinterState::PrinterStateError;

        if (printer_data.state_message == NULL || strcmp(printer_data.state_message, error))
        {
            printer_data.state_message = (char *)malloc(strlen(error) + 1);
            strcpy(printer_data.state_message, error);
        }
    }
}

void OctoPrinter::parse_file_list(JsonDocument &in, std::list<OctoFileSystemFile> &files, int fetch_limit)
{
    JsonArray result = in["files"];

    for (JsonObject file : result)
    {
        const char *path = file["path"];
        if (strcmp("local", file["origin"]))
        {
            LOG_F(("Skipping non-local file %s\n", path))
            continue;
        }

        OctoFileSystemFile f = {0};
        
        float modified = file["date"];
        auto file_iter = files.begin();

        while (file_iter != files.end())
        {
            if ((*file_iter).modified < modified)
                break;

            file_iter++;
        }

        if (file_iter == files.end() && files.size() >= fetch_limit)
            continue;

        f.name = (char *)malloc(strlen(path) + 1);
        if (f.name == NULL)
        {
            LOG_LN("Failed to allocate memory");
            continue;
        }
        strcpy(f.name, path);
        f.modified = modified;

        if (file_iter != files.end())
            files.insert(file_iter, f);
        else
            files.push_back(f);

        if (files.size() > fetch_limit)
        {
            auto last_entry = files.back();

            if (last_entry.name != NULL)
                free(last_entry.name);

            files.pop_back();
        }
    }
}
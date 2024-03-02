#pragma once

enum PRINTER_STATE {
    PRINTER_STATE_ERROR = 0,
    PRINTER_STATE_IDLE = 1,
    PRINTER_STATE_PRINTING = 2,
    PRINTER_STATE_PAUSED = 3,
};

extern const char* printerStateMessages[];

typedef struct {
    unsigned char state;
    char* stateMessage;
    float extruderTemp;
    float extruderTargetTemp;
    float bedTemp;
    float bedTargetTemp;
    float position[3];
    unsigned char canExtrude;
    unsigned char homedAxis;
    unsigned char absoluteCoords;
    float elapsedTime;
    float remainingTime;
    float filamentUsedMm;
    char* printFilename;
    float printProgress; // 0 -> 1
    float fanSpeed; // 0 -> 1
    float gcodeOffset[3];
    float speedMult;
    float extrudeMult;
    int totalLayers;
    int currentLayer;
    float pressureAdvance;
    float smoothTime;
    int feedrateMmPerS;
    int slicerEstimatedPrintTime;
} Printer;

extern Printer printer;
extern int klipperRequestConsecutiveFailCount;

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3

void DataLoop();
void DataSetup();
void SendGcode(bool wait, const char* gcode);
void MovePrinter(const char* axis, float amount, bool relative);

void FreezeRequestThread();
void UnfreezeRequestThread();

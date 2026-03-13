#pragma once
#include <cmath>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "pros/rtos.hpp"
#include "pros/misc.hpp"
#include "pros/misc.h"

/*
success = 0,
batteryLow = 1,
sdCardMissing = 2,
IMUCalibrationFailed = 3,
motorOverheat = 4
*/
enum class ErrorCode
{
    success = 0,
    batteryLow = 1,
    sdCardMissing = 2,
    IMUCalibrationFailed = 3,
    motorOverheat = 4
};

class XUtil
{
public:
    XUtil();

    pros::Controller master;
    pros::Controller partner;

    // Editable
    static const size_t frameMax = 10000;

    /*
    32 Byte Total
    Version: #
    Padding: {0}
    Delay Interval: #
    Expansion: {0}
    */
    struct CSettings
    {
        uint8_t version;
        uint8_t cPadding[3];
        uint32_t delayInterval;
        uint8_t cExpansion[24]; // For future expansion
    } __attribute__((packed));

    CSettings cSettings{1, {0}, 20, {0}}; // Manual Control Settings

    /*
    32 Byte Total
    Version: #
    Padding: {0}
    Delay Interval: #
    Padding: {0}
    */
    struct PSettings
    {
        uint8_t version;
        uint8_t pPadding[3];
        uint32_t delayInterval;
        uint8_t pExpansion[24]; // For future expansion
    } __attribute__((packed));

    PSettings pcSettings{2, {0}, 40, {0}}; // Playback Control settings
    PSettings pSettings;                   // Playback Replay Settings

    /*
    32 Byte Total
    type | offset | size | name
    double 0 8 rotation;
    int32_t 8 4 odomY;
    int16_t 12 2 leftV;
    int16_t 14 2 rightV;
    int16_t 16 2  intakeCMD;
    int16_t 18 2  outtakeBCMD;
    int16_t 20 2  outtakeTCMD;
    uint8_t 22 1 descoreCMD;
    uint8_t 23 1 armCMD;
    uint8_t 24 1 tareFlag;
    uint8_t 25 1 loadFlag;
    uint8_t 26 1 purePDFlag;
    uint8_t 27 1 noPDFlag;
    uint8_t 28 4 fExpansion[4];
    */
    struct PFrame
    {
        double rotation;
        int32_t odomY;
        int16_t leftV;
        int16_t rightV;
        int16_t intakeCMD;
        int16_t outtakeBCMD;
        int16_t outtakeTCMD;
        uint8_t descoreCMD;
        uint8_t armCMD;
        uint8_t tareFlag;
        uint8_t loadFlag;
        uint8_t purePDFlag;
        uint8_t noPDFlag;
        uint8_t fExpansion[4];
    } __attribute__((packed));

    std::vector<PFrame> frames;

    std::string selectedFile;

    std::vector<std::string> indexFiles(const char *path);
    void fileSelection();

    void checkSave(FILE *file, std::vector<XUtil::PFrame> &buffer);

    bool checkFile(FILE *file);
    void parseFile(FILE *file);

    void writeHeader(FILE *file);
    void readHeader(FILE *file);

    void handleError(ErrorCode error);

    void printPartnerControls();
};
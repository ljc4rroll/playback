#pragma once
#include <cmath>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "pros/rtos.hpp"
#include "pros/misc.hpp"

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
    };

    CSettings cSettings{1, {0}, 20, {0}};

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
    };

    PSettings pSettings;

    /*
    32 Byte Total
    double rotation
    int32 odomY
    int16 leftV
    int16 rightV
    int16 intake
    int16 outtakeB
    int16 outtakeT
    int8 descore
    int8 arm
    int8 8 Byte Expansion
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
        uint8_t fExpansion[8]; // Space for 8 Flags
    } __attribute__((packed));

    std::vector<PFrame> frames;

    std::string selectedFile;

    std::vector<std::string> indexFiles(const char *path);
    void fileSelection();

    void checkSave(FILE *file, std::vector<XUtil::PFrame> &buffer);

    bool checkFile(FILE *file);
    void parseFile(FILE *file);

    void writeHeader(FILE *file, const XUtil::PSettings &settings);
    XUtil::PSettings readHeader(FILE *file);

    void handleError(ErrorCode error);
};
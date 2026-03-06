#pragma once
#include "pros/imu.hpp"
#include "pros/rotation.hpp"
#include "pros/llemu.hpp"
#include "chassis.hpp"
#include "pneumatics.hpp"
#include "transfer.hpp"
#include "Xutil.hpp"

class Control
{
public:
    Control();

    Chassis chassis;
    Transfer transfer;
    Pneumatics pneumatics;
    XUtil xUtil;

    pros::Imu inertial;
    pros::Rotation odomY;

    // Autonomous Settings
    const double yKp = (5.0) / 10000.0; // Leave the "/ 10000.0"
    const double yKd = (15.0) / 10000.0; // Leave the "/ 10000.0"
    const double rKp = 5.0;
    const double rKd = 2.0;

    void reInitialize();
    void disabled();
    void checkMotorTemps();
    void setDataRates(bool autonomous); // If autonomous, uses pSettings. If opcontrol, uses cSettings.

    void manual(const XUtil::CSettings);
    void compManual(const XUtil::CSettings);
    void telemetryManual(const XUtil::CSettings);
    void recordManual(const XUtil::CSettings, std::vector<XUtil::PFrame> &buffer);

    void auton(const XUtil::PSettings PSettings, std::vector<XUtil::PFrame> &frames);
    void compAuton(const XUtil::PSettings PSettings, std::vector<XUtil::PFrame> &frames);
    void telemetryAuton(const XUtil::PSettings PSettings, std::vector<XUtil::PFrame> &frames);
};
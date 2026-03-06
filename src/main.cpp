#include "main.h"

pros::Controller master(pros::E_CONTROLLER_MASTER);
pros::Imu inertial(11);
pros::Rotation odomY(18);

XUtil xUtil;
Control control;
Playback playback;
Chassis chassis;
Transfer transfer;
Pneumatics pneumatics;

void initialize()
{
    if (!pros::usd::is_installed()) xUtil.handleError(ErrorCode::sdCardMissing);

    while (pros::battery::get_capacity() > 10.0)
    {
        playback.menu();
        control.checkMotorTemps();
    }
    exit(1);
}
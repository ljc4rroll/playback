#pragma once
#include <utility>
#include "pros/motors.hpp"

class Transfer
{
public:
    Transfer();

    void intake(bool intakeIn, bool intakeOut);
    void outtake(bool intakeOut, bool outtakeUp, bool outtakeDown);

    int_fast16_t intakeReturn(bool intakeIn, bool intakeOut);
    std::pair<int_fast16_t, int_fast16_t> outtakeReturn(bool intakeOut, bool outtakeUp, bool outtakeDown);

    void setBrakeMode(pros::motor_brake_mode_e brakeMode);

    pros::Motor intake_;
    pros::Motor outtakeB_;
    pros::Motor outtakeT_;

private:
    const int intakeInSpeed = 127;
    const int intakeOutSpeed = -90;
    const int outtakeBUpSpeed = -115;
    const int outtakeBDownSpeed = 55;
    const int outtakeTSpeed = 100;
};
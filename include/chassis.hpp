#pragma once
#include "pros/motor_group.hpp"

class Chassis
{
public:
    Chassis();

    void arcade(double vertical, double horizontal);
    void tank(double left, double right);

    void toggleSpeed();
    
    void setBrakeMode(pros::motor_brake_mode_e brakeMode);
    void setGearing(pros::motor_gearset_e gearset);

private:
    pros::MotorGroup leftMG_;
    pros::MotorGroup rightMG_;
};
#pragma once
#include "pros/motor_group.hpp"

class Chassis
{
public:
    Chassis();

    void arcade(double vertical, double horizontal);
    std::pair<double, double> arcadeReturn(double vertical, double horizontal);
    void tank(double left, double right);

    void toggleSpeed();
    
    void setBrakeMode(pros::motor_brake_mode_e brakeMode);
    void setGearing(pros::motor_gearset_e gearset);

    pros::MotorGroup leftMG_;
    pros::MotorGroup rightMG_;

    const double baseSpeed = 0.4;
    const double fastSpeed = 0.8;
    double currSpeedMult = baseSpeed;
};
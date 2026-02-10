#include "chassis.hpp"

Chassis::Chassis() : leftMG_({-10, -7, -1}),
                     rightMG_({20, 5, 14}) {}

std::pair<double, double> Chassis::arcade(double vertical, double horizontal)
{
    double leftV = (std::clamp((vertical + horizontal) * currSpeedMult, -127.0, 127.0));
    double rightV = (std::clamp((vertical - horizontal) * currSpeedMult, -127.0, 127.0));

    leftMG_.move(leftV);
    rightMG_.move(rightV);
    
    return std::pair<double, double>{leftV, rightV};
}

void Chassis::tank(double leftV, double rightV)
{
    leftMG_.move(std::clamp(leftV, -127.0, 127.0));
    rightMG_.move(std::clamp(rightV, -127.0, 127.0));
}

void Chassis::toggleSpeed()
{
    currSpeedMult = (currSpeedMult == baseSpeed) ? fastSpeed : baseSpeed;
}

void Chassis::setBrakeMode(pros::motor_brake_mode_e brakeMode)
{
    leftMG_.set_brake_mode(brakeMode);
    rightMG_.set_brake_mode(brakeMode);
}

void Chassis::setGearing(pros::motor_gearset_e gearset)
{
    leftMG_.set_gearing(gearset);
    rightMG_.set_gearing(gearset);
}
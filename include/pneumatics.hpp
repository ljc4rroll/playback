#pragma once
#include "pros/adi.hpp"

class Pneumatics
{
public:
    Pneumatics();

    void toggleDescore();
    void toggleArm();
    std::pair<bool, bool> getPistonState();
    void resetPistons();

    bool descoreExtended = false;
    bool armExtended = false;

    pros::adi::DigitalOut descore_;
    pros::adi::DigitalOut arm_;
};
#pragma once
#include "pros/adi.hpp"

class Pneumatics
{
public:
    Pneumatics();

    void togglePistonD();
    void togglePistonA();
    std::pair<bool, bool> getPistonState();
    void resetPistons();

    bool pistonDExtended = false;
    bool pistonAExtended = false;

    // Descore
    pros::adi::DigitalOut pistonD_;
    // Arm
    pros::adi::DigitalOut pistonA_;
};
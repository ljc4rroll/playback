#pragma once
#include "pros/adi.hpp"

class Pneumatics
{
public:
    Pneumatics();

    void toggleDescore();
    void setDescore(int8_t descoreState);
    void toggleArm();
    void setArm(int8_t descoreState);
    std::pair<bool, bool> getPistonState();
    void resetPistons();

    bool descoreExtended = false;
    bool armExtended = false;

    pros::adi::DigitalOut descore_;
    pros::adi::DigitalIn descoreIn_;
    pros::adi::DigitalOut arm_;
    pros::adi::DigitalIn armIn_;
};
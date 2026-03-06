#include "pneumatics.hpp"

Pneumatics::Pneumatics() : descore_('A'),
                           arm_('B') {}

void Pneumatics::toggleDescore()
{
    descoreExtended = !descoreExtended;
    descore_.set_value(descoreExtended);
}

void Pneumatics::setDescore(int8_t descoreState)
{
    descore_.set_value(descoreState);
}

void Pneumatics::toggleArm()
{
    armExtended = !armExtended;
    arm_.set_value(armExtended);
}

void Pneumatics::setArm(int8_t armState)
{
    arm_.set_value(armState);
}

std::pair<bool, bool> Pneumatics::getPistonState()
{
    return std::pair<bool, bool>(descoreExtended, armExtended);
}

void Pneumatics::resetPistons()
{
    descoreExtended = false;
    armExtended = false;
    descore_.set_value(0);
    arm_.set_value(0);
}
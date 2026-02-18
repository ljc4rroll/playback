#include "pneumatics.hpp"

Pneumatics::Pneumatics() : descore_('A'),
                           arm_('B') {}

void Pneumatics::toggleDescore()
{
    descoreExtended = !descoreExtended;
    descore_.set_value(descoreExtended);
}

void Pneumatics::toggleArm()
{
    armExtended = !armExtended;
    arm_.set_value(armExtended);
}

std::pair<bool, bool> Pneumatics::getPistonState()
{
    return std::pair<bool, bool>(descoreExtended, armExtended);
}

void Pneumatics::resetPistons()
{
    descore_.set_value(0);
    arm_.set_value(0);
}
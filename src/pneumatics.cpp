#include "pneumatics.hpp"

Pneumatics::Pneumatics() : pistonD_('D'),
                           pistonA_('A') {}

void Pneumatics::togglePistonD()
{
    pistonDExtended = !pistonDExtended;
    pistonD_.set_value(pistonDExtended);
}

void Pneumatics::togglePistonA()
{
    pistonAExtended = !pistonAExtended;
    pistonA_.set_value(pistonAExtended);
}

std::pair<bool, bool> Pneumatics::getPistonState()
{
    return std::pair<bool, bool>(pistonDExtended, pistonAExtended);
}

void Pneumatics::resetPistons()
{
    pistonD_.set_value(0);
    pistonA_.set_value(0);
}
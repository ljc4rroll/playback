#include "transfer.hpp"

Transfer::Transfer() : intake_(-2),
                       outtakeB_(9),
                       outtakeT_(-19) {}

void Transfer::intake(bool intakeIn, bool intakeOut)
{
    int intakeSpeed = intakeIn ? intakeInSpeed : intakeOut ? intakeOutSpeed : 0;
    intake_.move(intakeSpeed);
}

void Transfer::outtake(bool intakeOut, bool outtakeUp, bool outtakeDown)
{
    int bottomSpeed = 0;
    int topSpeed = 0;
    
    if (intakeOut) {
        bottomSpeed = 0;
        topSpeed = -outtakeTSpeed;
    } else if (outtakeUp) {
        bottomSpeed = outtakeBUpSpeed;
        topSpeed = outtakeTSpeed;
    } else if (outtakeDown) {
        bottomSpeed = outtakeBDownSpeed;
        topSpeed = -outtakeTSpeed;
    }
    
    outtakeB_.move(bottomSpeed);
    outtakeT_.move(topSpeed);
}

int_fast16_t Transfer::intakeReturn(bool intakeIn, bool intakeOut)
{

    int_fast16_t intakeSpeed = intakeIn ? intakeInSpeed : intakeOut ? 
    intakeOutSpeed : 0;
    intake_.move(intakeSpeed);
    return intakeSpeed;
}

std::pair<int_fast16_t, int_fast16_t> Transfer::outtakeReturn(bool intakeOut, bool outtakeUp, bool outtakeDown)
{
    int_fast16_t bottomSpeed = 0;
    int_fast16_t topSpeed = 0;
    
    if (intakeOut) {
        bottomSpeed = 0;
        topSpeed = -outtakeTSpeed;
    } else if (outtakeUp) {
        bottomSpeed = outtakeBUpSpeed;
        topSpeed = outtakeTSpeed;
    } else if (outtakeDown) {
        bottomSpeed = outtakeBDownSpeed;
        topSpeed = -outtakeTSpeed;
    }
    
    outtakeB_.move(bottomSpeed);
    outtakeT_.move(topSpeed);
    return {bottomSpeed, topSpeed};
}

void Transfer::setBrakeMode(pros::motor_brake_mode_e brakeMode)
{
    intake_.set_brake_mode(brakeMode);
    outtakeB_.set_brake_mode(brakeMode);
    outtakeT_.set_brake_mode(brakeMode);
}
#include "main.h"

pros::Controller master(pros::E_CONTROLLER_MASTER);

Chassis chassis;
Transfer transfer;
Pneumatics pneumatics;

void initialize() {
    if (pros::usd::is_installed() == 0)
        exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "INITIALIZING");
    pros::delay(50);

    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    chassis.setGearing(pros::E_MOTOR_GEAR_BLUE);

    transfer.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);

    pneumatics.resetPistons();
    pros::delay(20);
}

void disabled() {
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);

    chassis.tank(0.0, 0.0);
    transfer.intake(0, 0);
    transfer.outtake(0, 0, 0);
    pros::delay(20);
}

void competition_initialize() {
    if (pros::usd::is_installed() == 0)
        exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "INITIALIZING");
    pros::delay(50);

    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    chassis.setGearing(pros::E_MOTOR_GEAR_BLUE);

    transfer.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);

    pneumatics.resetPistons();
    pros::delay(20);
}

void opcontrol() {
    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "DRIVING");
    pros::delay(50);
    
    while (!master.get_digital(DIGITAL_DOWN))
    {
        // Handle chassis movement
        if (master.get_digital_new_press(DIGITAL_B))
            chassis.toggleSpeed();
        double vertical = master.get_analog(ANALOG_LEFT_Y);
        double horizontal = master.get_analog(ANALOG_RIGHT_X);

        chassis.arcade(vertical, horizontal);

        // Handle transfer system
        bool intakeIn = master.get_digital(DIGITAL_R1);
        bool intakeOut = master.get_digital(DIGITAL_R2);
        bool outtakeUp = master.get_digital(DIGITAL_L1);
        bool outtakeDown = master.get_digital(DIGITAL_L2);

        transfer.intake(intakeIn, intakeOut);
        transfer.outtake(intakeOut, outtakeUp, outtakeDown);

        // Handle pneumatics
        bool pistonDTriggered = master.get_digital_new_press(DIGITAL_X);
        bool pistonATriggered = master.get_digital_new_press(DIGITAL_A);

        if (pistonDTriggered)
            pneumatics.toggleDescore();
        if (pistonATriggered)
            pneumatics.toggleArm();

        pros::delay(20);
    }
    exit(0);
}
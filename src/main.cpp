#include "main.h"

pros::Controller master(pros::E_CONTROLLER_MASTER);
pros::MotorGroup left_mg({-10, -7, -1});
pros::MotorGroup right_mg({20, 5, 14});
pros::MotorGroup drivetrain({-10, 20, -7, 5, -1, 14});
pros::adi::DigitalOut piston_D('B'); // Descore
pros::adi::DigitalOut piston_A('A'); // Arm
pros::Motor intake(-2);
pros::Motor outtakeB(9); // Outtake bottom
pros::Motor outtakeT(-19); // Outtake top

void initialize() {
    pros::lcd::initialize();
    pros::lcd::set_text(0, "INITIALIZING");

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    left_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    right_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    left_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    left_mg.set_brake_mode(pros::E_MOTOR_BRAKE_BRAKE);
    right_mg.set_brake_mode(pros::E_MOTOR_BRAKE_BRAKE);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);

    piston_D.set_value(0);
    piston_A.set_value(0);
}

void disabled() {
    drivetrain.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    drivetrain.move(0);
    intake.move(0);
    outtakeB.move(0);
}

void competition_initialize() {
    pros::lcd::initialize();
    pros::lcd::set_text(0, "INITIALIZING");

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    left_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    right_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    left_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    left_mg.set_brake_mode(pros::E_MOTOR_BRAKE_BRAKE);
    right_mg.set_brake_mode(pros::E_MOTOR_BRAKE_BRAKE);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);

    piston_A.set_value(0);
    piston_D.set_value(0);
}

void autonomous() {
    FILE* file = fopen("/usd/misc.txt", "r");
    if (!file) {
        pros::lcd::set_text(1, "SD Missing / No File");
        exit;
    }

    double left_v, right_v;
    double intake_cmd, outtakeB_cmd;
    int piston_D_state;
    int piston_A_state;
    double target_rotation, current_rotation, rotation_difference;
    double kP = 0.4;
    double rotation_difference_bounds = 10;
    double motor_velocity_tolerance = .5;

    pros::lcd::set_text(0, "RECORDING ACTIVE");
    pros::delay(5);

	while (pros::competition::is_autonomous() && !pros::competition::is_disabled() && !feof(file) && master.get_digital(DIGITAL_DOWN) == 0)
	{
		fscanf(file, "%lf %lf %lf %lf %d %d\n", &left_v, &right_v, &intake_cmd, &outtakeB_cmd, &piston_D_state, &piston_A_state); // Add variable retrieval for rotation

        pros::delay(15);

        left_mg.move(left_v);
        right_mg.move(right_v);
        intake.move(intake_cmd);
        outtakeB.move(outtakeB_cmd);
        outtakeT.move(std::abs(outtakeB_cmd));
        piston_D.set_value(piston_D_state);
        piston_A.set_value(piston_A_state);

        pros::delay(35);
	}
    fclose(file);
    left_mg.move(0);
    right_mg.move(0);
    intake.move(0);
    outtakeB.move(0);

    pros::delay(5);
	pros::lcd::set_text(0, "ENDING PLAYBACK");
}

void opcontrol() {
    pros::lcd::set_text(0, "STARTING OPCONTROL");

    FILE* file = fopen("/usd/misc.txt", "w");

    float base_speed = 0.4f;
    float fast_speed = 0.8f;
    float curr_speed_mult = base_speed;
    int prev_speed_toggle = 0;
    int prev_pneumatic_D_toggle = 0;
    int prev_pneumatic_A_toggle = 0;
    bool pneumatics_D_extended = false;
    bool pneumatics_A_extended = false;
    double outtakeT_cmd = 0;
    double current_rotation = 0.0;

    pros::lcd::set_text(0, "RECORDING ACTIVE");
    pros::delay(10);
    
    while (master.get_digital(DIGITAL_DOWN) == 0) {
        int vertical = master.get_analog(ANALOG_LEFT_Y);
        int horizontal = master.get_analog(ANALOG_RIGHT_X);

        int intake_in = master.get_digital(DIGITAL_R1);
        int intake_out = master.get_digital(DIGITAL_R2);
        int outtakeB_up = master.get_digital(DIGITAL_L1);
        int outtakeB_down = master.get_digital(DIGITAL_L2);

        int pneumatics_D_triggered = master.get_digital(DIGITAL_A);
        int pneumatics_A_triggered = master.get_digital(DIGITAL_X);
        int speed_toggle = master.get_digital(DIGITAL_B);

        pros::delay(7);
        
        // Pneumatics DEScore toggle
        if (pneumatics_D_triggered && !prev_pneumatic_D_toggle) {
            pneumatics_D_extended = !pneumatics_D_extended;
            piston_D.set_value(pneumatics_D_extended);
        }

        // Pneumatics ARM toggle
        if (pneumatics_A_triggered && !prev_pneumatic_A_toggle) {
            pneumatics_A_extended = !pneumatics_A_extended;
            piston_A.set_value(pneumatics_A_extended);
        }

        // Speed toggle
        if (speed_toggle && !prev_speed_toggle) {
            curr_speed_mult = (curr_speed_mult == base_speed) ? fast_speed : base_speed;
        }

        prev_pneumatic_D_toggle = pneumatics_D_triggered;
        prev_pneumatic_A_toggle = pneumatics_A_triggered;
        prev_speed_toggle = speed_toggle;

        double left_voltage = (vertical + horizontal) * curr_speed_mult;
        double right_voltage = (vertical - horizontal) * curr_speed_mult;

        double intake_cmd = (intake_in - intake_out) * 127;
        double outtakeB_cmd = 0;
        if (outtakeB_up == 1 && outtakeB_down == 0) outtakeB_cmd = -127; else if (outtakeB_up == 0 && outtakeB_down == 1) outtakeB_cmd = 67;
        if (outtakeB_up == 1 || outtakeB_down == 1) outtakeT_cmd = 127; else outtakeT_cmd = 0;

        pros::delay(8);

        left_mg.move(left_voltage);
        right_mg.move(right_voltage);
        intake.move(intake_cmd);
        outtakeB.move(outtakeB_cmd);
        outtakeT.move(outtakeT_cmd);
        
        fprintf(file, "%lf %lf %lf %lf %d %d\n", left_voltage, right_voltage, intake_cmd, outtakeB_cmd, pneumatics_D_extended, pneumatics_A_extended);

        pros::delay(35);
    }
	fflush(file);
    if (file) fclose(file);
    left_mg.move(0);
    right_mg.move(0);
    intake.move(0);
    outtakeB.move(0);

    pros::delay(5);
	pros::lcd::set_text(0, "ENDING RECORDING");
}
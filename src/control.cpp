#include "control.hpp"

Control::Control() : inertial(11),
					 odomY(18) {}

void Control::reInitialize()
{
	xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, "RECALIBRATING");
	pros::delay(50);
	xUtil.master.set_text(1, 0, "DO NOT MOVE BOT");
	pros::delay(50);

	chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
	chassis.setGearing(pros::E_MOTOR_GEAR_BLUE);

	transfer.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);

	inertial.reset(true);
	if (!inertial.is_calibrating() && static_cast<pros::imu_status_e>(inertial.get_status()) != pros::E_IMU_STATUS_READY)
	{
		xUtil.handleError(ErrorCode::IMUCalibrationFailed);
	}
	odomY.reset_position();
	pneumatics.resetPistons();

	pros::lcd::initialize();

	pros::delay(100);
}

void Control::disabled()
{
	chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);
	chassis.tank(0.0, 0.0);

	transfer.intake(0, 0);
	transfer.outtake(0, 0, 0);

	pneumatics.setArm(0);
	pneumatics.setDescore(0);

	pros::delay(10);

	pros::lcd::shutdown();

	xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, "DISABLED");
	pros::delay(50);
}

void Control::checkMotorTemps()
{
	const double highTemp = 55.0;

	std::vector<int32_t> tempsL = chassis.leftMG_.is_over_temp_all();
	for (int32_t temp : tempsL)
	{
		if (temp == 1)
		{
			xUtil.master.clear();
			pros::delay(50);
			xUtil.master.set_text(1, 0, "LEFT DRIVETRAIN");
			pros::delay(50);
			xUtil.handleError(ErrorCode::motorOverheat);
		}
	}

	std::vector<int32_t> tempsR = chassis.rightMG_.is_over_temp_all();
	for (int32_t temp : tempsR)
	{
		if (temp == 1)
		{
			xUtil.master.clear();
			pros::delay(50);
			xUtil.master.set_text(1, 0, "RIGHT DRIVETRAIN");
			pros::delay(50);
			xUtil.handleError(ErrorCode::motorOverheat);
		}
	}

	if (transfer.intake_.is_over_temp())
	{
		xUtil.master.clear();
		pros::delay(50);
		xUtil.master.set_text(1, 0, "INTAKE");
		pros::delay(50);
		xUtil.handleError(ErrorCode::motorOverheat);
	}
	if (transfer.outtakeB_.is_over_temp())
	{
		xUtil.master.clear();
		pros::delay(50);
		xUtil.master.set_text(1, 0, "OUTTAKE B");
		pros::delay(50);
		xUtil.handleError(ErrorCode::motorOverheat);
	}
	if (transfer.outtakeT_.is_over_temp())
	{
		xUtil.master.clear();
		pros::delay(50);
		xUtil.master.set_text(1, 0, "OUTTAKE T");
		pros::delay(50);
		xUtil.handleError(ErrorCode::motorOverheat);
	}

	pros::delay(1000);
}

void Control::setDataRates(bool autonomous)
{
	if (autonomous)
	{
		inertial.set_data_rate(xUtil.pSettings.delayInterval);
		odomY.set_data_rate(xUtil.pSettings.delayInterval);
	}
	else
	{
		inertial.set_data_rate(xUtil.cSettings.delayInterval);
		odomY.set_data_rate(xUtil.cSettings.delayInterval);
	}
}

void Control::manual()
{
	while (!xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
	{
		// Handle chassis movement
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
			chassis.toggleSpeed();

		chassis.arcade(xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y), xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X));

		// Handle transfer system
		bool intakeOut = xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R2);
		int_fast16_t intakeCMD = transfer.intakeReturn(xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R1), intakeOut);
		std::pair<int_fast16_t, int_fast16_t> outtakeCMDs = transfer.outtakeReturn(intakeOut, xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L1), xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L2));

		// Handle pneumatics
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
			pneumatics.toggleDescore();
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
			pneumatics.toggleArm();

		pros::delay(xUtil.cSettings.delayInterval);
	}
}

void Control::compManual()
{
	while (!pros::competition::is_disabled)
	{
		// Handle chassis movement
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
			chassis.toggleSpeed();

		chassis.arcade(xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y), xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X));

		// Handle transfer system
		bool intakeOut = xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R2);
		int_fast16_t intakeCMD = transfer.intakeReturn(xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R1), intakeOut);
		std::pair<int_fast16_t, int_fast16_t> outtakeCMDs = transfer.outtakeReturn(intakeOut, xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L1), xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L2));

		// Handle pneumatics
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
			pneumatics.toggleDescore();
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
			pneumatics.toggleArm();
		pros::delay(xUtil.cSettings.delayInterval);
	}
}

void Control::telemetryManual() // WIP
{
	while (!xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
	{
		// Handle chassis movement
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
			chassis.toggleSpeed();

		chassis.arcade(xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y), xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X));

		// Handle transfer system
		bool intakeOut = xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R2);
		int_fast16_t intakeCMD = transfer.intakeReturn(xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R1), intakeOut);
		std::pair<int_fast16_t, int_fast16_t> outtakeCMDs = transfer.outtakeReturn(intakeOut, xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L1), xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L2));

		// Handle pneumatics
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
			pneumatics.toggleDescore();
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
			pneumatics.toggleArm();

		pros::lcd::print(0, "Odom: %f", odomY.get_position());
		pros::lcd::print(1, "Inertial: %f", inertial.get_rotation());

		pros::delay(xUtil.cSettings.delayInterval);
	}
}

void Control::recordManual(std::vector<XUtil::PFrame> &buffer)
{
	while (!xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
	{
		// Handle chassis movement
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
			chassis.toggleSpeed();

		std::pair<double, double> motorVs = chassis.arcadeReturn(xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y), xUtil.master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X));

		// Handle transfer system
		bool intakeOut = xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R2);

		int_fast16_t intakeCMD = transfer.intakeReturn(xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_R1), intakeOut);
		std::pair<int_fast16_t, int_fast16_t> outtakeCMDs = transfer.outtakeReturn(intakeOut, xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L1), xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_L2));

		// Handle pneumatics
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
			pneumatics.toggleDescore();
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
			pneumatics.toggleArm();
		std::pair<bool, bool> pistonsState = pneumatics.getPistonState();

		// Add inputs to buffer
		buffer.push_back({(double_t)inertial.get_rotation(),
						  (int32_t)odomY.get_position(),
						  (int16_t)motorVs.first,
						  (int16_t)motorVs.second,
						  (int16_t)intakeCMD,
						  (int16_t)outtakeCMDs.first,
						  (int16_t)outtakeCMDs.second,
						  (uint8_t)pistonsState.first,
						  (uint8_t)pistonsState.second,
						  (uint8_t)xUtil.partner.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A),
						  {}});

		pros::delay(xUtil.pcSettings.delayInterval);
	}
}

void Control::auton(std::vector<XUtil::PFrame> &frames) // WIP
{
	double yPreviousError = 0.0;
	double rPreviousError = 0.0;
	double yOffset = 0.0;
	double rOffset = 0.0;

	for (const auto &f : frames)
	{
		if (xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
			break;

		double odomYCurrentPosition = odomY.get_position();
		double currentRotation = inertial.get_rotation();

		if (f.tareFlag)
		{
			yOffset = odomYCurrentPosition - f.odomY;
			rOffset = currentRotation - f.rotation;
		}

		// Handle OdomY PD
		double yError = f.odomY - (odomYCurrentPosition - yOffset);
		double yDerivative = yError - yPreviousError;
		double yPositionCorrection = (yError * yKp) + (yDerivative * yKd);
		yPreviousError = yError;

		// Handle Rotation PD
		double rError = f.rotation - (currentRotation - rOffset);
		double rDerivative = rError - rPreviousError;
		double rotationCorrection = (rError * rKp) + (rDerivative * rKd);
		rPreviousError = rError;

		// Apply values
		chassis.tank((f.leftV + yPositionCorrection + rotationCorrection), (f.rightV + yPositionCorrection - rotationCorrection));
		transfer.intake_.move(f.intakeCMD);
		transfer.outtakeB_.move(f.outtakeBCMD);
		transfer.outtakeT_.move(f.outtakeTCMD);
		pneumatics.descore_.set_value(f.descoreCMD);
		pneumatics.arm_.set_value(f.armCMD);

		pros::delay(xUtil.pSettings.delayInterval);
	}
}

void Control::compAuton(std::vector<XUtil::PFrame> &frames) // WIP
{
	double yPreviousError = 0.0;
	double rPreviousError = 0.0;

	for (const auto &f : frames)
	{
		if (pros::competition::is_disabled())
			break;

		// Handle OdomY PD
		double odomYCurrentPosition = odomY.get_position();
		double yError = f.odomY - odomYCurrentPosition;
		double yDerivative = yError - yPreviousError;
		double yPositionCorrection = (yError * yKp) + (yDerivative * yKd);
		yPreviousError = yError;

		// Handle Rotation PD
		double currentRotation = inertial.get_rotation();
		double rError = f.rotation - currentRotation;
		double rDerivative = rError - rPreviousError;
		double rotationCorrection = (rError * rKp) + (rDerivative * rKd);
		rPreviousError = rError;

		// Apply values
		chassis.tank((f.leftV + yPositionCorrection + rotationCorrection), (f.rightV + yPositionCorrection - rotationCorrection));
		transfer.intake_.move(f.intakeCMD);
		transfer.outtakeB_.move(f.outtakeBCMD);
		transfer.outtakeT_.move(f.outtakeTCMD);
		pneumatics.descore_.set_value(f.descoreCMD);
		pneumatics.arm_.set_value(f.armCMD);

		pros::delay(xUtil.pSettings.delayInterval);
	}
}

void Control::telemetryAuton(std::vector<XUtil::PFrame> &frames) // WIP
{
	double yPreviousError = 0.0;
	double rPreviousError = 0.0;

	for (const auto &f : frames)
	{
		if (xUtil.master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
			break;

		// Handle OdomY PD
		double odomYCurrentPosition = odomY.get_position();
		double yError = f.odomY - odomYCurrentPosition;
		double yDerivative = yError - yPreviousError;
		double yPositionCorrection = (yError * yKp) + (yDerivative * yKd);
		yPreviousError = yError;

		// Handle Rotation PD
		double currentRotation = inertial.get_rotation();
		double rError = f.rotation - currentRotation;
		double rDerivative = rError - rPreviousError;
		double rotationCorrection = (rError * rKp) + (rDerivative * rKd);
		rPreviousError = rError;

		// Apply values
		chassis.tank((f.leftV + yPositionCorrection + rotationCorrection), (f.rightV + yPositionCorrection - rotationCorrection));
		transfer.intake_.move(f.intakeCMD);
		transfer.outtakeB_.move(f.outtakeBCMD);
		transfer.outtakeT_.move(f.outtakeTCMD);
		pneumatics.descore_.set_value(f.descoreCMD);
		pneumatics.arm_.set_value(f.armCMD);

		pros::lcd::print(0, "Odom: %f", yPositionCorrection);
		pros::lcd::print(1, "Inertial: %f", rotationCorrection);

		pros::delay(xUtil.pSettings.delayInterval);
	}
}
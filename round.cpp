#include "main.h"

pros::Controller master(pros::E_CONTROLLER_MASTER);
pros::Imu inertial(11);
pros::Rotation odomY(18);

Chassis chassis;
Transfer transfer;
Pneumatics pneumatics;

// Autonomous Settings
const double yKp = (0.5) / 10000.0; // Leave the "/ 10000.0"
const double yKd = (1.5) / 1000.0;  // Leave the "/ 10000.0"
const double rKp = 5.0;
const double rKd = 2.0;

std::vector<std::string> indexFiles(const char *path = "/")
{
    std::vector<std::string> files;
    static char buffer[1024];

    int32_t result = pros::usd::list_files(path, buffer, sizeof(buffer));
    char *token = std::strtok(buffer, "\n");
    pros::delay(20);
    while (token != nullptr)
    {
        std::string filename(token);
        size_t pos = filename.find(".txt");
        filename = filename.substr(0, pos);
        files.emplace_back(filename);
        token = std::strtok(nullptr, "\n");
    }
    pros::delay(20);

    return files;
}

struct InputFrame
{
    int16_t leftV;
    int16_t rightV;
    int16_t intakeCMD;
    int16_t outtakeBCMD;
    int16_t outtakeTCMD;
    uint8_t descoreCMD;
    uint8_t armCMD;
    int32_t odomYPosition;
    double rotation;
};

struct
{
    std::string selectedFile;
    std::vector<InputFrame> frames;
} playbackInfo;

void fileSelection()
{
    std::vector<std::string> files = indexFiles();
    int selectedIndex = 0;
    pros::delay(10);

    master.clear();
    pros::delay(50);

    while (!pros::competition::is_connected())
    {
        bool aPressed = master.get_digital_new_press(DIGITAL_A);
        bool upPressed = master.get_digital_new_press(DIGITAL_UP);
        bool downPressed = master.get_digital_new_press(DIGITAL_DOWN);
        pros::delay(10);

        if (aPressed)
        {
            playbackInfo.selectedFile = files[selectedIndex];
            pros::delay(10);
            break;
        }

        if (upPressed && selectedIndex > 0)
        {
            selectedIndex -= 1;
            master.clear_line(1);
            pros::delay(50);
        }
        else if (downPressed && selectedIndex < files.size() - 1)
        {
            selectedIndex += 1;
            master.clear_line(1);
            pros::delay(50);
        }

        if (selectedIndex > 0)
            master.set_text(0, 0, "^");
        else
            master.clear_line(0);
        pros::delay(50);
        master.set_text(1, 0, ("(A)" + files[selectedIndex]).c_str());
        pros::delay(50);
        if (selectedIndex < files.size() - 1)
            master.set_text(2, 0, "V");
        else
            master.clear_line(2);
        pros::delay(50);
    }
    pros::delay(10);
}

void recalibrate()
{
    if (pros::usd::is_installed() == 0)
        exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "RECALIBRATING");
    pros::delay(50);
    master.set_text(1, 0, "DO NOT MOVE BOT");
    pros::delay(50);

    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    chassis.setGearing(pros::E_MOTOR_GEAR_BLUE);

    transfer.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);

    inertial.reset(true);
    odomY.reset();

    pneumatics.resetPistons();
    pros::delay(20);
}

void initialize()
{
    if (pros::usd::is_installed() == 0)
        exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "NO FILE SELECTED");
    pros::delay(50);
    master.set_text(1, 0, "(X)CHANGE FILE");
    pros::delay(50);
    master.set_text(2, 0, "(A)CONTINUE");
    pros::delay(50);
    while (true)
    {
        if (master.get_digital_new_press(DIGITAL_X))
        {
            fileSelection();

            master.clear();
            pros::delay(50);
            if (playbackInfo.selectedFile.empty())
            {
                master.set_text(0, 0, "NO FILE SELECTED");
                pros::delay(50);
            }
            else
            {
                master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
                pros::delay(50);
            }
            master.set_text(1, 0, "(X)CHANGE FILE");
            pros::delay(50);
            master.set_text(2, 0, "(A)CONTINUE");
            pros::delay(50);
        }
        else if (master.get_digital_new_press(DIGITAL_A))
        {
            break;
        }
        pros::delay(10);
    }

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "INITIALIZING");
    pros::delay(50);

    if (!playbackInfo.selectedFile.empty())
    {
        FILE *file = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");

        // Find size of file
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        rewind(file);

        size_t frameCount = fileSize / sizeof(InputFrame);

        playbackInfo.frames.resize(frameCount);
        fread(playbackInfo.frames.data(), sizeof(InputFrame), frameCount, file);

        fclose(file);
    }

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "PLUG IN");
    pros::delay(50);
    if (playbackInfo.selectedFile.empty())
    {
        master.set_text(1, 0, "NO FILE");
        pros::delay(50);
    }
    else
    {
        master.set_text(1, 0, (playbackInfo.selectedFile).c_str());
        pros::delay(50);
    }

    while (!pros::competition::is_connected())
    {
        continue;
    }
}

void disabled()
{
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);

    chassis.tank(0.0, 0.0);
    transfer.intake(0, 0);
    transfer.outtake(0, 0, 0);
    pros::delay(20);
}

void competition_initialize()
{
    recalibrate();

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "STAGED");
    pros::delay(50);
    if (playbackInfo.selectedFile.empty())
    {
        master.set_text(1, 0, "NO FILE");
        pros::delay(50);
    }
    else
    {
        master.set_text(1, 0, (playbackInfo.selectedFile).c_str());
        pros::delay(50);
    }
}

void autonomous()
{
    double yPreviousError = 0.0;
    double rPreviousError = 0.0;

    for (const auto &f : playbackInfo.frames)
    {
        if (pros::competition::is_disabled())
            break;

        // Handle OdomY PD
        double odomYCurrentPosition = odomY.get_position();
        double yError = f.odomYPosition - odomYCurrentPosition;
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

        pros::delay(20);
    }

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "AUTON OVER");
    pros::delay(50);
}

void opcontrol()
{
    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "OPCONTROL");
    pros::delay(50);

    while (!pros::competition::is_disabled())
    {
        // Handle chassis movement
        if (master.get_digital_new_press(DIGITAL_B))
            chassis.toggleSpeed();

        chassis.arcade(master.get_analog(ANALOG_LEFT_Y), master.get_analog(ANALOG_RIGHT_X));

        // Handle transfer system
        bool intakeOut = master.get_digital(DIGITAL_R2);

        transfer.intake(master.get_digital(DIGITAL_R1), intakeOut);
        transfer.outtake(intakeOut, master.get_digital(DIGITAL_L1), master.get_digital(DIGITAL_L2));

        // Handle pneumatics
        if (master.get_digital_new_press(DIGITAL_X))
            pneumatics.toggleDescore();
        if (master.get_digital_new_press(DIGITAL_A))
            pneumatics.toggleArm();

        pros::delay(20);
    }
    exit(0);
}
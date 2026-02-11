#include "main.h"

pros::Controller master(pros::E_CONTROLLER_MASTER);
pros::Imu inertial(11);

Chassis chassis;
Transfer transfer;
Pneumatics pneumatics;

// Autonomous Settings
const double rKp = 0.2;
const double rKd = 0.3;

std::vector<std::string> indexFiles(const char *path = "/")
{
    std::vector<std::string> files;
    static char buffer[1024];

    int32_t result = pros::usd::list_files(path, buffer, sizeof(buffer)); // Puts the list of file names into buffer
    if (result < 0)
    {
        return files;
    }
    char *token = std::strtok(buffer, "\n");
    pros::delay(20);
    while (token != nullptr)
    {
        std::string filename(token);
        files.emplace_back(filename);
        token = std::strtok(nullptr, "\n");
    }
    pros::delay(20);

    return files;
}

// Used for recording inputs. The buffer is stored in RAM to avoid timing inconsistencies during runtime.
struct InputFrame
{
    // Use 16-bit for smaller memory footprint
    int16_t leftV;
    int16_t rightV;
    int16_t intakeCMD;
    int16_t outtakeBCMD;
    int16_t outtakeTCMD;
    uint8_t pistonDCMD;
    uint8_t pistonACMD;
    double rotation;
};

// Used for GUI and playback selection
struct
{
    std::string selectedFile;
} playbackInfo;

void fileSelection()
{
    std::vector<std::string> files = indexFiles();
    if (files.empty())
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILES FOUND");
        pros::delay(3000);
        return;
    }

    int selectedIndex = 0;
    pros::delay(10);

    master.clear();
    pros::delay(50);

    while (true)
    {
        if (master.get_digital_new_press(DIGITAL_A))
        {
            playbackInfo.selectedFile = files[selectedIndex];
            break;
        }
        if (master.get_digital_new_press(DIGITAL_B))
            return;
        pros::delay(10);

        if (master.get_digital_new_press(DIGITAL_UP) && selectedIndex > 0)
        {
            selectedIndex -= 1;
            master.clear_line(1);
            pros::delay(50);
        }
        else if (master.get_digital_new_press(DIGITAL_DOWN) && selectedIndex < files.size() - 1)
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

void checkSave(FILE *file, const std::vector<InputFrame> &buffer)
{
    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "(A)SAVE RECORDING");
    pros::delay(50);
    master.set_text(1, 0, "(B)CANCEL");
    pros::delay(50);

    while (true)
    {
        bool aPressed = master.get_digital_new_press(DIGITAL_A);
        bool bPressed = master.get_digital_new_press(DIGITAL_B);

        if (aPressed)
        {
            if (!file)
            {
                master.clear();
                pros::delay(50);
                master.set_text(0, 0, "FILE ERROR");
                pros::delay(3000);
                return;
            }
            master.clear();
            pros::delay(50);
            master.set_text(0, 0, "WRITING");
            pros::delay(50);
            master.set_text(1, 0, "DO NOT KILL");
            pros::delay(50);
            fwrite(buffer.data(), sizeof(InputFrame), buffer.size(), file);
            fclose(file);
            pros::delay(50);
            return;
        }
        if (bPressed)
        {
            pros::delay(20);
            return;
        }
        pros::delay(20);
    }
}

void reInitialize()
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

    pneumatics.resetPistons();
    pros::delay(20);
}

void autonomous(std::vector<InputFrame> &frames)
{
    double rPreviousError = 0.0;

    for (const auto &f : frames)
    {
        if (master.get_digital(DIGITAL_DOWN))
            break;

        double currentRotation = inertial.get_rotation();
        double rError = f.rotation - currentRotation;
        double rDerivative = rError - rPreviousError;
        double rotationCorrection = (rError * rKp) + (rDerivative * rKd);
        rPreviousError = rError;

        chassis.tank((f.leftV + rotationCorrection), (f.rightV - rotationCorrection));
        transfer.intake_.move(f.intakeCMD);
        transfer.outtakeB_.move(f.outtakeBCMD);
        transfer.outtakeT_.move(f.outtakeTCMD);
        pneumatics.pistonD_.set_value(f.pistonDCMD);
        pneumatics.pistonA_.set_value(f.pistonACMD);

        pros::delay(20);
    }
}

void opcontrol(std::vector<InputFrame> &buffer)
{
    while (!master.get_digital(DIGITAL_DOWN))
    {
        // Handle chassis movement
        if (master.get_digital_new_press(DIGITAL_B))
            chassis.toggleSpeed();
        double vertical = master.get_analog(ANALOG_LEFT_Y);
        double horizontal = master.get_analog(ANALOG_RIGHT_X);

        std::pair<double, double> motorVs = chassis.arcadeReturn(vertical, horizontal);

        // Handle transfer system
        bool intakeIn = master.get_digital(DIGITAL_R1);
        bool intakeOut = master.get_digital(DIGITAL_R2);
        bool outtakeUp = master.get_digital(DIGITAL_L1);
        bool outtakeDown = master.get_digital(DIGITAL_L2);

        int_fast16_t intakeCMD = transfer.intakeReturn(intakeIn, intakeOut);
        std::pair<int_fast16_t, int_fast16_t> outtakeCMDs = transfer.outtakeReturn(intakeOut, outtakeUp, outtakeDown);

        // Handle pneumatics
        bool pistonDTriggered = master.get_digital_new_press(DIGITAL_A);
        bool pistonATriggered = master.get_digital_new_press(DIGITAL_X);

        if (pistonDTriggered)
            pneumatics.togglePistonD();
        if (pistonATriggered)
            pneumatics.togglePistonA();
        std::pair<bool, bool> pistonsState = pneumatics.getPistonState();

        // Add inputs to buffer
        buffer.push_back({(int16_t)motorVs.first,
                          (int16_t)motorVs.second,
                          (int16_t)intakeCMD,
                          (int16_t)outtakeCMDs.first,
                          (int16_t)outtakeCMDs.second,
                          (uint8_t)pistonsState.first,
                          (uint8_t)pistonsState.second,
                          (double_t)inertial.get_rotation()});

        pros::delay(20);
    }
}

void replay()
{
    reInitialize();

    if (playbackInfo.selectedFile.empty())
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE *fileR = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");
    if (!fileR)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "FILE ERROR");
        pros::delay(50);
        return;
    }

    // Find size of file
    fseek(fileR, 0, SEEK_END);
    size_t fileSize = ftell(fileR);
    rewind(fileR);

    size_t frameCount = fileSize / sizeof(InputFrame);

    // Create vector of inputFrames
    std::vector<InputFrame> frames(frameCount);
    fread(frames.data(), sizeof(InputFrame), frameCount, fileR);
    fclose(fileR);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    pros::delay(50);
    master.set_text(1, 0, "REPLAY");
    pros::delay(1000);

    autonomous(frames);
    disabled();
    pros::delay(10);

    master.clear();
    pros::delay(50);
}

void overwrite()
{
    reInitialize();

    // Check if file exists
    if (playbackInfo.selectedFile.empty())
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE *fileW = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "wb");
    if (!fileW)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "FILE ERROR");
        pros::delay(50);
        return;
    }

    // Sets the amount of input frames that can be recorded.
    // Calculate by multiplying the amount of seconds by the total delay of the driver control loop.
    constexpr size_t frameMax = 6000;
    std::vector<InputFrame> buffer;
    buffer.reserve(frameMax);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    pros::delay(50);
    master.set_text(1, 0, "OVERWRITE");
    pros::delay(50);

    opcontrol(buffer);
    disabled();

    // Cleanup
    checkSave(fileW, buffer);
    buffer.clear();
    pros::delay(10);

    master.clear();
    pros::delay(50);
}

void extend()
{
    reInitialize();

    if (playbackInfo.selectedFile.empty())
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE *fileR = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");
    if (!fileR)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "FILE ERROR");
        pros::delay(50);
        return;
    }
    FILE *fileA = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "ab");
    if (!fileA)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "FILE ERROR");
        pros::delay(50);
        return;
    }

    // Find size of file
    fseek(fileR, 0, SEEK_END);
    size_t fileSize = ftell(fileR);
    rewind(fileR);
    size_t frameCount = fileSize / sizeof(InputFrame);

    // Create vector of inputFrames
    std::vector<InputFrame> frames(frameCount);
    fread(frames.data(), sizeof(InputFrame), frameCount, fileR);
    fclose(fileR);

    // Sets the amount of input frames that can be recorded.
    // Calculate by multiplying amount of seconds by the total delay of the driver control loop.
    constexpr size_t frameMax = 6000;
    std::vector<InputFrame> buffer;
    buffer.reserve(frameMax);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    pros::delay(50);
    master.set_text(1, 0, "EXTEND");
    pros::delay(50);

    autonomous(frames);
    opcontrol(buffer);
    disabled();

    checkSave(fileA, buffer);

    // Cleanup
    buffer.clear();
    pros::delay(10);

    master.clear();
    pros::delay(50);
}

void playback()
{
    master.clear();
    pros::delay(50);
    if (playbackInfo.selectedFile.empty())
    {
        master.set_text(0, 0, "NO FILE SELECTED");
    }
    else
    {
        master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    }
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
            }
            else
            {
                master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
            }
            pros::delay(50);
            master.set_text(1, 0, "(X)CHANGE FILE");
            pros::delay(50);
            master.set_text(2, 0, "(A)CONTINUE");
            pros::delay(50);
        }
        else if (master.get_digital_new_press(DIGITAL_A))
        {
            break;
        }
        else if (master.get_digital_new_press(DIGITAL_B))
        {
            exit(0);
        }
        pros::delay(10);
    }
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "(X)AUTONOMOUS");
    pros::delay(50);
    master.set_text(1, 0, "(Y)OVERWRITE");
    pros::delay(50);
    master.set_text(2, 0, "(A)EXTEND");
    pros::delay(50);

    while (true)
    {
        if (master.get_digital_new_press(DIGITAL_B))
        {
            break;
        }
        else if (master.get_digital_new_press(DIGITAL_X))
        {
            replay();
            break;
        }
        else if (master.get_digital_new_press(DIGITAL_Y))
        {
            overwrite();
            break;
        }
        else if (master.get_digital_new_press(DIGITAL_A))
        {
            extend();
            break;
        }
        pros::delay(30);
    }
    pros::delay(10);
}

void initialize()
{
    reInitialize();

    while (pros::battery::get_capacity() > 10.0)
    {
        playback();
        disabled();
        pros::delay(10);
    }
    exit(1);
}

void disabled()
{
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);

    chassis.tank(0.0, 0.0);
    transfer.intake(0, 0);
    transfer.outtake(0, 0, 0);
    pros::delay(20);
}
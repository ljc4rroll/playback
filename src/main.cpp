#include "main.h"
#include <vector>
#include <cstring>

pros::Controller master(pros::E_CONTROLLER_MASTER);
pros::MotorGroup leftMg({-10, -7, -1});
pros::MotorGroup rightMg({20, 5, 14});
pros::MotorGroup drivetrain({-10, 20, -7, 5, -1, 14});
pros::adi::DigitalOut pistonD('B'); // Descore
pros::adi::DigitalOut pistonA('A'); // Arm
pros::Motor intake(-2);
pros::Motor outtakeB(9); // Outtake bottom
pros::Motor outtakeT(-19); // Outtake top
pros::Imu inertial(11);

std::vector<std::string> indexFiles(const char* path = "/") {
    std::vector<std::string> files;
    static char buffer[1024];

    int32_t result = pros::usd::list_files(path, buffer, sizeof(buffer)); // Puts the list of file names into buffer
    if (result < 0) {
        return files;
    }
    char* token = std::strtok(buffer, "\n");
    pros::delay(20);
    while (token != nullptr) {
        std::string filename(token);
        files.emplace_back(filename);
        token = std::strtok(nullptr, "\n");
    }
    pros::delay(20);

    return files;
}

// Used for recording inputs. The buffer is stored in RAM to avoid timing inconsistencies during runtime.
struct InputFrame {
    // Use 16-bit for smaller memory footprint
    int16_t leftV;
    int16_t rightV;
    int16_t intakeCmd;
    int16_t outtakeBCmd;
    int16_t outtakeTCmd;
    uint8_t pistonD;
    uint8_t pistonA;
    double rotation;
};

// Used for GUI and playback selection
struct {
    std::string selectedFile;
    std::vector<InputFrame> frames;
} playbackInfo;

void fileSelection() {
    std::vector<std::string> files = indexFiles();
    if (files.empty()) {
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

    while (true) {
        if (master.get_digital_new_press(DIGITAL_A)) {
            playbackInfo.selectedFile = files[selectedIndex]; 
            break;
        }
        if (master.get_digital_new_press(DIGITAL_B)) return;
        pros::delay(10);

        if (master.get_digital_new_press(DIGITAL_UP) && selectedIndex > 0) {
            selectedIndex -= 1; 
            master.clear_line(1);
            pros::delay(50);
        }
        else if (master.get_digital_new_press(DIGITAL_DOWN) && selectedIndex < files.size() - 1) {
            selectedIndex += 1;
            master.clear_line(1);
            pros::delay(50);
        }
        
        if (selectedIndex > 0) master.set_text(0, 0, "^"); else master.clear_line(0);
        pros::delay(50);
        master.set_text(1, 0, ("(A)" + files[selectedIndex]).c_str()); 
        pros::delay(50);
        if (selectedIndex < files.size() - 1) master.set_text(2, 0, "V"); else master.clear_line(2); 
        pros::delay(50);
    }
    pros::delay(10);
}

bool checkSave() {
    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "(A)SAVE RECORDING");
    pros::delay(50);
    master.set_text(1, 0, "(B)CANCEL");
    pros::delay(50);

    while (true) {
        bool aPressed = master.get_digital_new_press(DIGITAL_A);
        bool bPressed = master.get_digital_new_press(DIGITAL_B);
        
        if (aPressed) return true;
        if (bPressed) return false;
        pros::delay(20);
    }
}

void reInitialize() {
    if (pros::usd::is_installed() == 0) exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "RECALIBRATING");
    pros::delay(50);
    master.set_text(1, 0, "DO NOT MOVE BOT");
    pros::delay(50);

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    leftMg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    rightMg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    leftMg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    rightMg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    leftMg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    rightMg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeT.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    
    inertial.reset(true);

    pistonA.set_value(0);
    pistonD.set_value(0);
    pros::delay(100);
}

void autonomous() {
    reInitialize();

    // Check if file exists
    if (playbackInfo.selectedFile.empty()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE* file = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");
    if (!file) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "FILE ERROR");
        pros::delay(3000);
        return;
    }
    
    // Find size of file
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    size_t frameCount = fileSize / sizeof(InputFrame);

    // Create vector of inputFrames
    playbackInfo.frames.resize(frameCount);
    fread(playbackInfo.frames.data(), sizeof(InputFrame), frameCount, file);
    
    double rKp = 0.2;
    double rKd = 0.3;
    double previousError = 0.0;

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    pros::delay(50);
    master.set_text(1, 0, "AUTONOMOUS");
    pros::delay(1000);

    for (const auto& f : playbackInfo.frames) {
        if (master.get_digital(DIGITAL_DOWN)) break;

        double currentRotation = inertial.get_rotation();
        double error = f.rotation - currentRotation;
        double derivative = error - previousError;
        double correction = (error * rKp) + (derivative * rKd);
        
        leftMg.move(std::clamp(f.leftV + correction, -127.0, 127.0));
        rightMg.move(std::clamp(f.rightV - correction, -127.0, 127.0));
        intake.move(f.intakeCmd);
        outtakeB.move(f.outtakeBCmd);
        outtakeT.move(f.outtakeTCmd);
        pistonD.set_value(f.pistonD);
        pistonA.set_value(f.pistonA);

        previousError = error;

        pros::delay(20);
    }
    fclose(file);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "ENDING");
    pros::delay(50);
}

void overwrite() {    
    reInitialize();

    // Check if file exists
    if (playbackInfo.selectedFile.empty()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    
    // Sets the amount of input frames that can be recorded. 
    // Calculate by multiplying the amount of seconds by the total delay of the driver control loop. 
    constexpr size_t frameMax = 6000;
    std::vector<InputFrame> buffer;
    buffer.reserve(frameMax);

    float baseSpeed = 0.4f;
    float fastSpeed = 0.8f;
    float currSpeedMult = baseSpeed;
    bool pistonDExtended = false;
    bool pistonAExtended = false;
    double outtakeTCmd = 0;
    pros::delay(20);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    pros::delay(50);
    master.set_text(1, 0, "OVERWRITE");
    pros::delay(50);
    
    while (!master.get_digital(DIGITAL_DOWN)) {
        int vertical = master.get_analog(ANALOG_LEFT_Y);
        int horizontal = master.get_analog(ANALOG_RIGHT_X);

        int intakeIn = master.get_digital(DIGITAL_R1);
        int intakeOut = master.get_digital(DIGITAL_R2);
        int outtakeBUp = master.get_digital(DIGITAL_L1);
        int outtakeBDown = master.get_digital(DIGITAL_L2);

        int pistonDTriggered = master.get_digital_new_press(DIGITAL_A);
        int pistonATriggered = master.get_digital_new_press(DIGITAL_X);
        int speedToggle = master.get_digital_new_press(DIGITAL_B);
        
        // Pneumatics DEScore toggle
        if (pistonDTriggered) {
            pistonDExtended = !pistonDExtended;
            pistonD.set_value(pistonDExtended);
        }

        // Pneumatics ARM toggle
        if (pistonATriggered) {
            pistonAExtended = !pistonAExtended;
            pistonA.set_value(pistonAExtended);
        }

        // Speed toggle
        if (speedToggle) {
            currSpeedMult = (currSpeedMult == baseSpeed) ? fastSpeed : baseSpeed;
        }

        double leftVoltage = std::clamp((vertical + horizontal) * currSpeedMult, -127.0f, 127.0f);
        double rightVoltage = std::clamp((vertical - horizontal) * currSpeedMult, -127.0f, 127.0f);

        double intakeCmd = 0;
        if (intakeIn == 1 && intakeOut == 0) intakeCmd = 127; else if (intakeIn == 0 && intakeOut == 1) intakeCmd = -100;
        double outtakeBCmd = 0;
        if (outtakeBUp == 1 && outtakeBDown == 0) outtakeBCmd = -127; else if (outtakeBUp == 0 && outtakeBDown == 1) outtakeBCmd = 67;
        double outtakeTCmd = 0;
        if (outtakeBUp == 1 || outtakeBDown == 1) outtakeTCmd = 127;

        leftMg.move(leftVoltage);
        rightMg.move(rightVoltage);
        intake.move(intakeCmd);
        outtakeB.move(outtakeBCmd);
        outtakeT.move(outtakeTCmd);
        
        buffer.push_back({
            (int16_t)leftVoltage,
            (int16_t)rightVoltage,
            (int16_t)intakeCmd,
            (int16_t)outtakeBCmd,
            (int16_t)outtakeTCmd,
            (uint8_t)pistonDExtended,
            (uint8_t)pistonAExtended,
            (double_t)inertial.get_rotation()
        });

        pros::delay(20);
    }
    disabled();
    pros::delay(20);

    
    if (checkSave()) {
        FILE* file = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "wb");
        if (!file) {
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
    }
    pros::delay(10);

    master.clear();
    pros::delay(50);
}

void extend() {
    reInitialize();

    // Check if file exists
    if (playbackInfo.selectedFile.empty()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE* file = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");
    if (!file) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "FILE ERROR");
        pros::delay(3000);
        return;
    }

    // Find size of file
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    size_t frameCount = fileSize / sizeof(InputFrame);

    // Create vector of inputFrames
    playbackInfo.frames.resize(frameCount);
    fread(playbackInfo.frames.data(), sizeof(InputFrame), frameCount, file);

    // Sets the amount of input frames that can be recorded. 
    // Calculate by multiplying amount of seconds by the total delay of the driver control loop. 
    constexpr size_t frameMax = 6000;
    std::vector<InputFrame> buffer;
    buffer.reserve(frameMax);

    float baseSpeed = 0.4f;
    float fastSpeed = 0.8f;
    float currSpeedMult = baseSpeed;
    bool pistonDExtended = false;
    bool pistonAExtended = false;

    double rKp = 0.2;
    double rKd = 0.3;
    double previousError = 0.0;
    pros::delay(20);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    pros::delay(50);
    master.set_text(1, 0, "EXTEND");
    pros::delay(50);

    for (const auto& f : playbackInfo.frames) {
        if (master.get_digital(DIGITAL_DOWN)) break;

        double currentRotation = inertial.get_rotation();
        double error = f.rotation - currentRotation;
        double derivative = error - previousError;
        double correction = (error * rKp) + (derivative * rKd);
        
        leftMg.move(std::clamp(f.leftV + correction, -127.0, 127.0));
        rightMg.move(std::clamp(f.rightV - correction, -127.0, 127.0));
        intake.move(f.intakeCmd);
        outtakeB.move(f.outtakeBCmd);
        outtakeT.move(f.outtakeTCmd);
        pistonD.set_value(f.pistonD);
        pistonA.set_value(f.pistonA);

        previousError = error;

        pros::delay(20);
    }
    while (!master.get_digital(DIGITAL_DOWN)) {
        int vertical = master.get_analog(ANALOG_LEFT_Y);
        int horizontal = master.get_analog(ANALOG_RIGHT_X);

        int intakeIn = master.get_digital(DIGITAL_R1);
        int intakeOut = master.get_digital(DIGITAL_R2);
        int outtakeBUp = master.get_digital(DIGITAL_L1);
        int outtakeBDown = master.get_digital(DIGITAL_L2);

        int pistonDTriggered = master.get_digital_new_press(DIGITAL_A);
        int pistonATriggered = master.get_digital_new_press(DIGITAL_X);
        int speedToggle = master.get_digital_new_press(DIGITAL_B);
        
        // Pneumatics DEScore toggle
        if (pistonDTriggered) {
            pistonDExtended = !pistonDExtended;
            pistonD.set_value(pistonDExtended);
        }

        // Pneumatics ARM toggle
        if (pistonATriggered) {
            pistonAExtended = !pistonAExtended;
            pistonA.set_value(pistonAExtended);
        }

        // Speed toggle
        if (speedToggle) {
            currSpeedMult = (currSpeedMult == baseSpeed) ? fastSpeed : baseSpeed;
        }

        double leftVoltage = std::clamp((vertical + horizontal) * currSpeedMult, -127.0f, 127.0f);
        double rightVoltage = std::clamp((vertical - horizontal) * currSpeedMult, -127.0f, 127.0f);

        double intakeCmd = 0;
        if (intakeIn == 1 && intakeOut == 0) intakeCmd = 127; else if (intakeIn == 0 && intakeOut == 1) intakeCmd = -100;
        double outtakeBCmd = 0;
        if (outtakeBUp == 1 && outtakeBDown == 0) outtakeBCmd = -127; else if (outtakeBUp == 0 && outtakeBDown == 1) outtakeBCmd = 67;
        double outtakeTCmd = 0;
        if (outtakeBUp == 1 || outtakeBDown == 1) outtakeTCmd = 127;

        leftMg.move(leftVoltage);
        rightMg.move(rightVoltage);
        intake.move(intakeCmd);
        outtakeB.move(outtakeBCmd);
        outtakeT.move(outtakeTCmd);
        
        buffer.push_back({
            (int16_t)leftVoltage,
            (int16_t)rightVoltage,
            (int16_t)intakeCmd,
            (int16_t)outtakeBCmd,
            (int16_t)outtakeTCmd,
            (uint8_t)pistonDExtended,
            (uint8_t)pistonAExtended,
            (double_t)inertial.get_rotation()
        });

        pros::delay(20);
    }
    disabled();
    pros::delay(20);
    
    if (checkSave()) {
        FILE* fileW = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "ab");
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "WRITING TO FILE");
        pros::delay(50);
        master.set_text(1, 0, "DO NOT KILL PROGRAM");
        pros::delay(50);
        fwrite(buffer.data(), sizeof(InputFrame), buffer.size(), fileW);
        fclose(fileW);
        pros::delay(50);
    }
    fclose(file);
    pros::delay(10);

    master.clear();
    pros::delay(50);
}

void playback() {
    master.clear();
    pros::delay(50);
    if (playbackInfo.selectedFile.empty()) {
        master.set_text(0, 0, "NO FILE SELECTED"); 
    } else {
        master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
    }
    pros::delay(50);
    master.set_text(1, 0, "(X)CHANGE FILE");
    pros::delay(50);
    master.set_text(2, 0, "(A)CONTINUE");
    pros::delay(50);
    while (true) {
        if (master.get_digital_new_press(DIGITAL_X)) {
            fileSelection();
            master.clear();
            pros::delay(50);
            if (playbackInfo.selectedFile.empty()) {
                master.set_text(0, 0, "NO FILE SELECTED"); 
            } else {
                master.set_text(0, 0, (playbackInfo.selectedFile).c_str());
            }
            pros::delay(50);
            master.set_text(1, 0, "(X)CHANGE FILE");
            pros::delay(50);
            master.set_text(2, 0, "(A)CONTINUE");
            pros::delay(50);
        } else if (master.get_digital_new_press(DIGITAL_A)) {
            break;
        } else if (master.get_digital_new_press(DIGITAL_B)) {
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

    while (true) {
        if (master.get_digital_new_press(DIGITAL_B)) {
            break;
        } else if (master.get_digital_new_press(DIGITAL_X)) {
            autonomous();
            break;
        } else if (master.get_digital_new_press(DIGITAL_Y)) {
            overwrite();
            break;
        } else if (master.get_digital_new_press(DIGITAL_A)) {
            extend();
            break;
        }
        pros::delay(30);
    }
}

void initialize() {
    // Check if microSD is inserted
    if (pros::usd::is_installed() == 0) exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "CALIBRATING");
    pros::delay(50);
    master.set_text(1, 0, "DO NOT MOVE BOT");
    pros::delay(50);

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    leftMg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    rightMg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    leftMg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    rightMg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    leftMg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    rightMg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeT.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    
    pistonA.set_value(0);
    pistonD.set_value(0);
    pros::delay(20);

    inertial.reset(true);

    while (pros::battery::get_capacity() > 10.0)
    {
        playback();
        disabled();
        pros::delay(10);
    }
    exit(1);
}

void disabled() {
    drivetrain.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    
    leftMg.move(0);
    rightMg.move(0);
    intake.move(0);
    outtakeB.move(0);
    outtakeT.move(0);
    return;
}

void competition_initialize() {
    // Check if microSD is inserted
    if (pros::usd::is_installed() == 0) exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "CALIBRATING");
    pros::delay(50);
    master.set_text(1, 0, "DO NOT MOVE BOT");
    pros::delay(50);

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    leftMg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    rightMg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    leftMg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    rightMg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    leftMg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    rightMg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeT.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    
    pistonA.set_value(0);
    pistonD.set_value(0);
    pros::delay(20);

    inertial.reset(true);

    while (pros::battery::get_capacity() > 10.0)
    {
        playback();
        disabled();
        pros::delay(10);
    }
    exit(1);
}
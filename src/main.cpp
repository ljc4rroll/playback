#include "main.h"
#include <vector>
#include <filesystem>
#include <cstring>

pros::Controller master(pros::E_CONTROLLER_MASTER);
pros::MotorGroup left_mg({-10, -7, -1});
pros::MotorGroup right_mg({20, 5, 14});
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

    int32_t result = pros::usd::list_files(path, buffer, sizeof(buffer));
    char* token = std::strtok(buffer, "\n");
    pros::delay(20);
    while (token != nullptr) {
        std::string filename(token);
        size_t pos = filename.find(".txt");
        filename = filename.substr(0, pos);
        files.emplace_back(filename);
        token = std::strtok(nullptr, "\n");
    }
    pros::delay(20);

    return files;
}

// Used for GUI and playback selection
struct {
    std::string selectedFile;
} playbackInfo;

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
};

void fileSelection() {
    std::vector<std::string> files = indexFiles();
    int selectedIndex = 0;
    pros::delay(10);

    master.clear();
    pros::delay(50);

    while (true) {
        bool aPressed = master.get_digital_new_press(DIGITAL_A);
        bool bPressed = master.get_digital_new_press(DIGITAL_B);
        bool upPressed = master.get_digital_new_press(DIGITAL_UP);
        bool downPressed = master.get_digital_new_press(DIGITAL_DOWN);

        if (aPressed) {
            playbackInfo.selectedFile = files[selectedIndex]; 
            break;
        }
        if (bPressed) return;
        pros::delay(10);

        if (upPressed && selectedIndex > 0) {
            selectedIndex -= 1; 
            master.clear_line(1);
            pros::delay(50);
        }
        else if (downPressed && selectedIndex < files.size() - 1) {
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

void autonomous() {
    // Check if file exists
    if (playbackInfo.selectedFile.empty()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE* file = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");
    // if (!file) exit(3);
    
    // Find size of file
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    size_t frameCount = fileSize / sizeof(InputFrame);

    // Create vector of inputFrames
    std::vector<InputFrame> frames(frameCount);
    fread(frames.data(), sizeof(InputFrame), frameCount, file);
    
    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "PLAYBACK");
    pros::delay(1000);

    // Check if still in autonomous mode, is not disabled, and the frames have not ended 
    for (const auto& f : frames) {
        if (master.get_digital_new_press(DIGITAL_DOWN)) break;

        pros::delay(10);
        
        left_mg.move(f.leftV);
        right_mg.move(f.rightV);
        intake.move(f.intakeCmd);
        outtakeB.move(f.outtakeBCmd);
        outtakeT.move(f.outtakeTCmd);
        pistonD.set_value(f.pistonD);
        pistonA.set_value(f.pistonA);

        pros::delay(10);
    }
    fclose(file);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "ENDING");
    pros::delay(1000);
}

void overwrite() {    
    // Check if file exists
    if (playbackInfo.selectedFile.empty()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    
    // Sets the amount of input frames that can be recorded. 
    // Calculate by multiplying amount of seconds by the total delay of the driver control loop. 
    constexpr size_t frameMax = 6000;
    std::vector<InputFrame> buffer;
    buffer.reserve(frameMax);

    float base_speed = 0.4f;
    float fast_speed = 0.8f;
    float curr_speed_mult = base_speed;
    bool pistonDExtended = false;
    bool pistonAExtended = false;
    double outtakeTCmd = 0;
    pros::delay(20);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "OVERWRITE");
    pros::delay(50);
    
    while (!master.get_digital_new_press(DIGITAL_DOWN)) {
        int vertical = master.get_analog(ANALOG_LEFT_Y);
        int horizontal = master.get_analog(ANALOG_RIGHT_X);

        int intake_in = master.get_digital(DIGITAL_R1);
        int intake_out = master.get_digital(DIGITAL_R2);
        int outtakeB_up = master.get_digital(DIGITAL_L1);
        int outtakeB_down = master.get_digital(DIGITAL_L2);

        int pistonDTriggered = master.get_digital_new_press(DIGITAL_A);
        int pistonATriggered = master.get_digital_new_press(DIGITAL_X);
        int speed_toggle = master.get_digital_new_press(DIGITAL_B);

        pros::delay(5);
        
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
        if (speed_toggle) {
            curr_speed_mult = (curr_speed_mult == base_speed) ? fast_speed : base_speed;
        }

        double leftVoltage = std::clamp((vertical + horizontal) * curr_speed_mult, -127.0f, 127.0f);
        double rightVoltage = std::clamp((vertical - horizontal) * curr_speed_mult, -127.0f, 127.0f);

        double intakeCmd = (intake_in - intake_out) * 127;
        double outtakeBCmd = 0;
        if (outtakeB_up == 1 && outtakeB_down == 0) outtakeBCmd = -127; else if (outtakeB_up == 0 && outtakeB_down == 1) outtakeBCmd = 67;
        if (outtakeB_up == 1 || outtakeB_down == 1) outtakeTCmd = 127; else outtakeTCmd = 0;

        pros::delay(5);

        left_mg.move(leftVoltage);
        right_mg.move(rightVoltage);
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
            (uint8_t)pistonAExtended
        });

        pros::delay(10);
    }
    disabled();
    pros::delay(20);

    
    if (checkSave()) {
        FILE* file = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "wb");
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
    // Check if file exists
    if (playbackInfo.selectedFile.empty()) {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(3000);
        return;
    }
    FILE* fileR = fopen(("/usd/" + playbackInfo.selectedFile).c_str(), "rb");
    if (!fileR) exit(3);

    // Find size of file
    fseek(fileR, 0, SEEK_END);
    size_t fileSize = ftell(fileR);
    rewind(fileR);

    size_t frameCount = fileSize / sizeof(InputFrame);

    // Create vector of inputFrames
    std::vector<InputFrame> frames(frameCount);
    fread(frames.data(), sizeof(InputFrame), frameCount, fileR);

    // Sets the amount of input frames that can be recorded. 
    // Calculate by multiplying amount of seconds by the total delay of the driver control loop. 
    constexpr size_t frameMax = 6000;
    std::vector<InputFrame> buffer;
    buffer.reserve(frameMax);

    float base_speed = 0.4f;
    float fast_speed = 0.8f;
    float curr_speed_mult = base_speed;
    bool pistonDExtended = false;
    bool pistonAExtended = false;
    double outtakeTCmd = 0;
    pros::delay(20);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "EXTEND");
    pros::delay(50);

    // Check if still in autonomous mode, is not disabled, and the frames have not ended 
    for (const auto& f : frames) {
        pros::delay(10);
        
        left_mg.move(f.leftV);
        right_mg.move(f.rightV);
        intake.move(f.intakeCmd);
        outtakeB.move(f.outtakeBCmd);
        outtakeT.move(f.outtakeTCmd);
        pistonD.set_value(f.pistonD);
        pistonA.set_value(f.pistonA);

        pros::delay(10);
    }
    while (!master.get_digital_new_press(DIGITAL_DOWN)) {
        int vertical = master.get_analog(ANALOG_LEFT_Y);
        int horizontal = master.get_analog(ANALOG_RIGHT_X);

        int intake_in = master.get_digital(DIGITAL_R1);
        int intake_out = master.get_digital(DIGITAL_R2);
        int outtakeB_up = master.get_digital(DIGITAL_L1);
        int outtakeB_down = master.get_digital(DIGITAL_L2);

        int pistonDTriggered = master.get_digital_new_press(DIGITAL_A);
        int pistonATriggered = master.get_digital_new_press(DIGITAL_X);
        int speed_toggle = master.get_digital_new_press(DIGITAL_B);

        pros::delay(5);
        
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
        if (speed_toggle) {
            curr_speed_mult = (curr_speed_mult == base_speed) ? fast_speed : base_speed;
        }

        double leftVoltage = std::clamp((vertical + horizontal) * curr_speed_mult, -127.0f, 127.0f);
        double rightVoltage = std::clamp((vertical - horizontal) * curr_speed_mult, -127.0f, 127.0f);

        double intakeCmd = (intake_in - intake_out) * 127;
        double outtakeBCmd = 0;
        if (outtakeB_up == 1 && outtakeB_down == 0) outtakeBCmd = -127; else if (outtakeB_up == 0 && outtakeB_down == 1) outtakeBCmd = 67;
        if (outtakeB_up == 1 || outtakeB_down == 1) outtakeTCmd = 127; else outtakeTCmd = 0;

        pros::delay(5);

        left_mg.move(leftVoltage);
        right_mg.move(rightVoltage);
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
            (uint8_t)pistonAExtended
        });

        pros::delay(10);
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
    fclose(fileR);
    pros::delay(10);

    master.clear();
    pros::delay(50);
}

void playback() {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "(X)CHANGE FILE");
        pros::delay(50);
        master.set_text(2, 0, "(A)CONTINUE");
        pros::delay(50);
    while (true) {
        bool xPressed = master.get_digital_new_press(DIGITAL_X);
        bool aPressed = master.get_digital_new_press(DIGITAL_A);
        bool bPressed = master.get_digital_new_press(DIGITAL_B);
        if (xPressed) {
            fileSelection();
            master.clear();
            pros::delay(50);
            master.set_text(0, 0, "(X)CHANGE FILE");
            pros::delay(50);
            // master.set_text(1, 0, "(Y)CREATE/DELETE");
            pros::delay(50);
            master.set_text(2, 0, "(A)CONTINUE");
            pros::delay(50);
        } else if (aPressed) {
            break;
        } else if (bPressed) {
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
        bool xPressed = master.get_digital_new_press(DIGITAL_X);
        bool yPressed = master.get_digital_new_press(DIGITAL_Y);
        bool aPressed = master.get_digital_new_press(DIGITAL_A);
        bool bPressed = master.get_digital_new_press(DIGITAL_B);

        if (bPressed) {
            break;
        }
        if (xPressed) {
            autonomous();
            break;
        }
        if (yPressed) {
            overwrite();
            break;
        }
        if (aPressed) {
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
    master.set_text(0, 0, "INITIALIZING");
    pros::delay(50);

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    left_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    right_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    left_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    left_mg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    right_mg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    
    pistonA.set_value(0);
    pistonD.set_value(0);
    pros::delay(100);

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
    
    left_mg.move(0);
    right_mg.move(0);
    intake.move(0);
    outtakeB.move(0);
    outtakeT.move(0);
}

void competition_initialize() {
    // Check if microSD is inserted
    if (pros::usd::is_installed() == 0) exit(2);
    pros::delay(10);

    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "INITIALIZING");
    pros::delay(50);

    drivetrain.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    left_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);
    right_mg.set_gearing(pros::E_MOTOR_GEAR_BLUE);

    drivetrain.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    left_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);
    right_mg.set_encoder_units(pros::E_MOTOR_ENCODER_DEGREES);

    left_mg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    right_mg.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    intake.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    outtakeB.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    
    pistonA.set_value(0);
    pistonD.set_value(0);
    pros::delay(100);

    while (pros::battery::get_capacity() > 10.0)
    {
        playback();
        disabled();
        pros::delay(10);
    }
    exit(1);
}
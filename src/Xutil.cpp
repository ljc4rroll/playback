#include "Xutil.hpp"

XUtil::XUtil() : master(pros::E_CONTROLLER_MASTER) {}

std::vector<std::string> XUtil::indexFiles(const char *path = "/")
{
    std::vector<std::string> files;
    static char buffer[1024];

    int32_t result = pros::usd::list_files(path, buffer, sizeof(buffer));
    if (result < 0)
    {
        return files;
    }
    char *token = strtok(buffer, "\n");
    pros::delay(20);
    while (token != nullptr)
    {
        std::string filename(token);
        files.emplace_back(filename);
        token = strtok(nullptr, "\n");
    }
    pros::delay(20);

    return files;
}

void XUtil::fileSelection()
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
        if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
        {
            selectedFile = files.at(selectedIndex);
            break;
        }
        if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
            return;
        pros::delay(10);

        if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP) && selectedIndex > 0)
        {
            selectedIndex -= 1;
            master.clear_line(1);
            pros::delay(50);
        }
        else if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN) && selectedIndex < files.size() - 1)
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
        master.set_text(1, 0, ("(A)" + files.at(selectedIndex)).c_str());
        pros::delay(50);
        if (selectedIndex < files.size() - 1)
            master.set_text(2, 0, "V");
        else
            master.clear_line(2);
        pros::delay(50);
    }
    pros::delay(10);
}

void XUtil::checkSave(FILE *file, std::vector<XUtil::PFrame> &buffer)
{
    master.clear();
    pros::delay(50);
    master.set_text(0, 0, "(A)SAVE RECORDING");
    pros::delay(50);
    master.set_text(1, 0, "(B)CANCEL");
    pros::delay(50);

    while (true)
    {
        bool aPressed = master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A);
        bool bPressed = master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B);

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
            fwrite(buffer.data(), sizeof(XUtil::PFrame), buffer.size(), file);
            pros::delay(1000);
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

bool XUtil::checkFile(FILE *file)
{
    if (selectedFile.empty())
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NO FILE");
        pros::delay(50);
        master.set_text(1, 0, selectedFile.c_str());
        pros::delay(50);
        master.rumble(".");
        pros::delay(5000);
        return false;
    }
    if (!file)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "NULL POINTER");
        pros::delay(50);
        master.set_text(1, 0, selectedFile);
        pros::delay(50);
        master.rumble(".");
        pros::delay(5000);
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "EMPTY FILE");
        pros::delay(50);
        master.set_text(1, 0, selectedFile);
        pros::delay(50);
        master.rumble(".");
        pros::delay(5000);
        return false;
    }
    /*
    
    size_t fileSize = ftell(file);
    if (fileSize < sizeof(XUtil::PSettings))
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "INVALID HEADER");
        pros::delay(50);
        master.set_text(1, 0, selectedFile);
        pros::delay(50);
        master.rumble(".");
        pros::delay(5000);
        return false;
    }
    size_t remainingSize = fileSize - sizeof(XUtil::PSettings);
    if (remainingSize % sizeof(XUtil::PFrame) != 0)
    {
        master.clear();
        pros::delay(50);
        master.set_text(0, 0, "INVALID FRAMES");
        pros::delay(50);
        master.set_text(1, 0, selectedFile);
        pros::delay(50);
        master.rumble(".");
        pros::delay(5000);
        return false;
    }

    fseek(file, 0, SEEK_SET);
    */
    return true;
}

void XUtil::parseFile(FILE *file)
{
    pSettings = readHeader(file);

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, sizeof(XUtil::PSettings), SEEK_SET);

    size_t frameCount = (fileSize - sizeof(XUtil::PSettings)) / sizeof(XUtil::PFrame);

    frames.resize(frameCount);
    fread(frames.data(), sizeof(XUtil::PFrame), frameCount, file);
}

void XUtil::writeHeader(FILE *file, const XUtil::PSettings &settings)
{
    fwrite(&settings, sizeof(XUtil::PSettings), 1, file);
}

XUtil::PSettings XUtil::readHeader(FILE *file)
{
    XUtil::PSettings settings;
    fread(&settings, sizeof(XUtil::PSettings), 1, file);
    return settings;
}

void XUtil::handleError(ErrorCode error)
{
    switch(error)
    {
        case ErrorCode::batteryLow:
            master.clear();
            master.set_text(0, 0, "LOW BATTERY");
            master.rumble(".");
            break;
        case ErrorCode::sdCardMissing:
            master.clear();
            master.set_text(0, 0, "NO SD CARD");
            master.rumble(".");
            break;
        case ErrorCode::IMUCalibrationFailed:
            master.clear();
            master.set_text(0, 0, "IMU FAIL");
            master.rumble(".");
            break;
        case ErrorCode::motorOverheat:
            master.set_text(0, 0, "OVERHEATED MOTOR");
            master.rumble(".");    
            break;
        default:
            master.clear();
            master.set_text(0, 0, "UNKNOWN ERROR");
            master.rumble(".");    
            break;
        pros::delay(5000);
        exit(static_cast<int>(error));
    }
}
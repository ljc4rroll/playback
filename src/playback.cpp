#include "playback.hpp"

Playback::Playback() {}

void Playback::menu()
{
	xUtil.master.clear();
	pros::delay(50);
	if (xUtil.selectedFile.empty())
	{
		xUtil.master.set_text(0, 0, "NO FILE SELECTED");
	}
	else
	{
		xUtil.master.set_text(0, 0, (xUtil.selectedFile).c_str());
	}
	pros::delay(50);

	xUtil.master.set_text(1, 0, "(X)CHANGE FILE");
	pros::delay(50);
	xUtil.master.set_text(2, 0, "(A)CONTINUE");
	pros::delay(50);

	while (true)
	{
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
		{
			xUtil.fileSelection();
			xUtil.master.clear();
			pros::delay(50);
			if (xUtil.selectedFile.empty())
			{
				xUtil.master.set_text(0, 0, "NO FILE SELECTED");
			}
			else
			{
				xUtil.master.set_text(0, 0, (xUtil.selectedFile).c_str());
			}
			pros::delay(50);
			xUtil.master.set_text(1, 0, "(X)CHANGE FILE");
			pros::delay(50);
			xUtil.master.set_text(2, 0, "(A)CONTINUE");
			pros::delay(50);
		}
		else if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
		{
			break;
		}
		else if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
		{
			exit(0);
		}
		pros::delay(10);
	}
	pros::delay(10);

	xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, "(X)AUTONOMOUS");
	pros::delay(50);
	xUtil.master.set_text(1, 0, "(Y)OVERWRITE");
	pros::delay(50);
	xUtil.master.set_text(2, 0, "(A)EXTEND");
	pros::delay(50);

	while (true)
	{
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
		{
			break;
		}
		else if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
		{
			replay();
			break;
		}
		else if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_Y))
		{
			overwrite();
			break;
		}
        else if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A))
		{
			extend();
			break;
		}
		pros::delay(30);
	}
	pros::delay(10);
}

void Playback::roundMenu()
{
	xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, "NO FILE SELECTED");
	pros::delay(50);
	xUtil.master.set_text(1, 0, "(X)CHANGE FILE");
	pros::delay(50);
	xUtil.master.set_text(2, 0, "(A)CONTINUE");
	pros::delay(50);

	while (!pros::competition::is_connected())
	{
		if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X))
		{
			xUtil.fileSelection();
			xUtil.master.clear();
			pros::delay(50);
			if (xUtil.selectedFile.empty())
			{
				xUtil.master.set_text(0, 0, "NO FILE SELECTED");
			}
			else
			{
				xUtil.master.set_text(0, 0, (xUtil.selectedFile).c_str());
				pros::delay(50);
				xUtil.master.set_text(2, 0, "PLUG IN");
			}
			pros::delay(50);
			xUtil.master.set_text(1, 0, "(X)CHANGE FILE");
			pros::delay(50);
		}
		else if (xUtil.master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B))
		{
			exit(0);
		}
		pros::delay(10);
	}
	pros::delay(10);
}

void Playback::replay()
{	
	FILE *file = fopen(("/usd/" + xUtil.selectedFile).c_str(), "rb");
    if (!xUtil.checkFile(file))
    {
		fclose(file);
        return;
    }
	
    xUtil.parseFile(file);
    fclose(file);
    pros::delay(10);
	
	control.reInitialize();
    control.setDataRates(true);
	
    xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, (xUtil.selectedFile).c_str());
	pros::delay(50);
	xUtil.master.set_text(1, 0, "REPLAY");
    xUtil.master.rumble(".");
	pros::delay(1000);

	xUtil.master.print(0, 0, "Delay: %d", xUtil.pSettings.delayInterval);
	pros::delay(2000);

    control.telemetryAuton(xUtil.pSettings, xUtil.frames);
    control.disabled();

    xUtil.frames.clear();
    pros::delay(10);
}

void Playback::overwrite()
{
	FILE *file = fopen(("/usd/" + xUtil.selectedFile).c_str(), "wb");
    if (!xUtil.checkFile(file))
    {
		fclose(file);
        return;
    }
	
	std::vector<XUtil::PFrame> buffer;
	buffer.reserve(XUtil::frameMax);
    pros::delay(10);
	
	control.reInitialize();
    
	xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, (xUtil.selectedFile).c_str());
	pros::delay(50);
	xUtil.master.set_text(1, 0, "OVERWRITE");
    xUtil.master.rumble(".");
	pros::delay(1000);

    control.setDataRates(false);
    control.recordManual(buffer);
    control.disabled();

    xUtil.checkSave(file, buffer);

    buffer.clear();
    pros::delay(10);
}

void Playback::extend()
{
	
	FILE *file = fopen(("/usd/" + xUtil.selectedFile).c_str(), "ab");
    if (!xUtil.checkFile(file) && !xUtil.checkFile(file))
    {
		fclose(file);
        return;
    }
	
    xUtil.parseFile(file);
    std::vector<XUtil::PFrame> buffer;
	buffer.reserve(XUtil::frameMax);
    pros::delay(10);
	
	control.reInitialize();

    xUtil.master.clear();
	pros::delay(50);
	xUtil.master.set_text(0, 0, (xUtil.selectedFile).c_str());
	pros::delay(50);
	xUtil.master.set_text(1, 0, "EXTEND");
    xUtil.master.rumble(".");
	pros::delay(1000);

    control.setDataRates(false);
    control.auton(xUtil.frames);
    control.setDataRates(true);
    control.recordManual(buffer);
    control.disabled();

    xUtil.checkSave(file, buffer);

    fclose(file);
    xUtil.frames.clear();
    buffer.clear();
    pros::delay(10);
}
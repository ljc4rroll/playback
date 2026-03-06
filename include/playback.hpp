#pragma once
#include "Xutil.hpp"
#include "control.hpp"

class Playback
{
public:
	Playback();

	XUtil xUtil;
	Control control;

	void menu();

	void replay();
	void overwrite();
	void extend();
};
#pragma once
#include <ctime>
#include "Common.h"

class Timer
{
private:
	Logger * _logger = NULL;
	void timerThread();
	clock_t startTime;
public:
	static unsigned long currentTime;
	void startTimer();
	Timer();
	~Timer();
};


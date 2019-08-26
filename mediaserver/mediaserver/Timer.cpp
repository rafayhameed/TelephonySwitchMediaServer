#include "Timer.h"

thread *checkDelayTimerThread;
unsigned long Timer::currentTime;;

Timer::Timer()
{
	_logger = GlobalLogger::StaticLogger;
}


Timer::~Timer()
{
}


void Timer::timerThread()
{
	try
	{
		_logger->info("Timer::delayProgram", "Timer Thread started");
		while (Common::keepRunning)
		{
			this_thread::sleep_for(std::chrono::seconds(TimerSleep));
			currentTime++;
		}
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Timer::timerThread", " Error in Timer: " + string(ex.__cxa_exception_type()->name()));
	}
}

void Timer::startTimer()
{
	checkDelayTimerThread = new thread(&Timer::timerThread,this);
	Common::threadVect->push_back(checkDelayTimerThread);
	currentTime = 0;
}




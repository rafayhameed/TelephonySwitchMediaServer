#pragma once
#include "Common.h"
#include"MediaServer.h"

class LoadApp
{
private:
	Logger * _logger = NULL;
//	MediaServer mediaServer;
public:
	void StartLoadApp();
	void LoadTestingThread();
	LoadApp();
	~LoadApp();
};


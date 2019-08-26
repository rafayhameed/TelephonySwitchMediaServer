#pragma once
#include <vector>
#include <iostream>
#include "MediaServer.h"
#include "Configurations.h"
#include "Prompt.h"

class Load
{
private:
	Logger * _logger = NULL;

public:

	Load();
	~Load();
	bool makePackets(int promptID, std::string promptName);
	bool loadprompts();
	void unloadPrompt(int promptID);
};


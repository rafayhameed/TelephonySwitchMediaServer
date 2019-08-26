#pragma once
#include "Common.h"
#include "Timer.h"
#include "rtp_target.h"

class rtp_target;

class PromptPort
{
private:
	Logger * _logger = NULL;
	

public:
	Common::playMessage* messageDetail;

	bool free = false;
	unsigned long startTime = 0;
	rtp_target* rtpTarget;
	Common::ENResponseCode response = Common::RESP_SUCCESS;
	int packetCount;

	bool setPort(Common::playMessage* messageObj);
	void resetPort();
	PromptPort();
	~PromptPort();
};


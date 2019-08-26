#pragma once

#include <arpa/inet.h>
#include <string>
#include <iostream>
#include "Common.h"

class rtp_prompt
{
private:
	Logger * _logger = NULL;

public:
	bool remove = false;
	void *playpromptthread(void * parg);
	void fun();
	unsigned long packetCount;
	rtp_prompt();
	~rtp_prompt();
	bool load(const std::string promptfile, unsigned int payloadType);
	bool unload();
	Common::rtp_packet_t * getPacket(unsigned long & Index);
};


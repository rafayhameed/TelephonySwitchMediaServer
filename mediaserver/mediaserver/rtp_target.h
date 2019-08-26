#pragma once
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "MediaServer.h"
class rtp_target
{
private:
	Logger * _logger = NULL;
	rtp_prompt* packetPtr;
	unsigned long packetCount;

public:
	void setdest(const std::string ip, const unsigned short port);
	void setsource(const std::string ip, const unsigned short port);
//	bool startplayback(const int promptindex);
	int sendnextpacket(int sockfd);
	Common::rtp_packet_t * getnextpacket();
	int getPromptIndex();
	bool findprompt(int promptID, std::string codecType);
	unsigned long getPacketCount();

	rtp_target();
	~rtp_target();

	protected:
		
		int m_promptIndex;
		unsigned long m_promptdata_nextindex;
};


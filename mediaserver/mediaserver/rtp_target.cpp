#include "rtp_target.h"

sockaddr_in m_dest;
sockaddr_in m_source;

int rtp_target::getPromptIndex() 
{
	return m_promptIndex;
}

void rtp_target::setdest(const std::string ip, const unsigned short port)
{
	m_dest.sin_port = htons(port);
	m_dest.sin_addr.s_addr = inet_addr(ip.c_str());
}
void rtp_target::setsource(const std::string ip, const unsigned short port)
{
	m_dest.sin_port = htons(port);
	m_dest.sin_addr.s_addr = inet_addr(ip.c_str());
}
	
sockaddr * getdest()
{
	return (sockaddr *)&m_dest;
}
sockaddr * getsource()
{
	return (sockaddr *)&m_source;
}
	
int rtp_target::sendnextpacket(int sockfd)
{
	Common::rtp_packet_t * ptr = getnextpacket();
	/*if (!ptr)
		return -1;
	*/
	ssize_t datasent = sendto(sockfd, ptr, sizeof(ptr->hdr) + sizeof(ptr->data), 0, (struct sockaddr*) &m_dest, sizeof(sockaddr)); //later also use the source sockaddr 
	if (datasent == 0)
	{
		_logger->error("rtp_target::sendnextpacket", "Packet sending Failed for  : " + std::to_string(m_promptdata_nextindex));
		return -1;
	}
	return m_promptdata_nextindex;
}
	
bool rtp_target::findprompt(int promptID, std::string codecType)
{
	try {
		packetPtr = (*MediaServer::promptsMap->find(promptID)->second->getPromptPackets())[std::distance(std::begin(Configurations::_CodecTypesMap), Configurations::_CodecTypesMap.find(codecType))];
		if (!packetPtr)
		{
			_logger->error("rtp_target::findprompt", "Prompt packet not found for specified Codec " + codecType);
			return false;
		}
		packetCount = packetPtr->packetCount;
		m_promptdata_nextindex = 0;
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("rtp_target::findprompt", " Exception " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
			
}

unsigned long rtp_target::getPacketCount()
{
	return packetCount;
}



Common::rtp_packet_t * rtp_target::getnextpacket()
{
	return packetPtr->getPacket(m_promptdata_nextindex);
}


rtp_target::rtp_target()
{
	m_promptIndex = -1;
	m_promptdata_nextindex = 0;
	_logger = GlobalLogger::StaticLogger;
}


rtp_target::~rtp_target()
{
}

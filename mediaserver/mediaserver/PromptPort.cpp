#include "PromptPort.h"


bool PromptPort::setPort(Common::playMessage* messageObj)
{
	try
	{
		startTime = Timer::currentTime;
		messageDetail = messageObj;

		rtpTarget->setdest(messageDetail->IP, messageDetail->port);
		if (/*!(rtpTarget->startplayback(messageDetail->promptID) && */!rtpTarget->findprompt(messageDetail->promptID,messageDetail->codecType))
		{
			response = Common::RESP_MSG_FORMAT_ERROR;
			return false;
		}
		packetCount = rtpTarget->getPacketCount() - 1;
		free = false;
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PromptPort::setPort", " Exception ... " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
	
}

void PromptPort::resetPort()
{
	free = true;
	messageDetail->stop = true;
}

PromptPort::PromptPort()
{
	_logger = GlobalLogger::StaticLogger;
	rtpTarget = new rtp_target();
}


PromptPort::~PromptPort()
{
	delete rtpTarget;
}

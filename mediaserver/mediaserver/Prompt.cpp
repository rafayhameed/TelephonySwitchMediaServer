#include "Prompt.h"

std::vector<rtp_prompt*>* Prompt::getPromptPackets()
{
	return promptData;
}

void Prompt::setPromptPackets(std::vector<rtp_prompt*>* promptObj)
{
	
}

int Prompt::getRefCount()
{
	return refCount;
}



bool Prompt::decrementRefCount()
{
	refCount--;
	if (remove)
	{
		promptLock.lock();
		if(refCount == 0)
			stopAssinging = true;
		promptLock.unlock();		
		return true;
	}
	return false;
}

bool Prompt::incrementRefCount()
{
	bool result;
	promptLock.lock();
	result = stopAssinging;
	promptLock.unlock();
	if (result)
		result = false;
	refCount++;
	return true;
}

bool Prompt::getStopAssigning()
{
	return stopAssinging;
}

void Prompt::setStopAssigning(bool stop)
{
	stopAssinging = stop;
}

Prompt::Prompt(int ID, std::vector<rtp_prompt*>* promptObj)
{
	_logger = GlobalLogger::StaticLogger;
	promptData = promptObj;
	promptID = ID;
}


Prompt::~Prompt()
{
	try
	{
		_logger->info("Prompt::~Prompt", "unloading Prompt");
		for (rtp_prompt *rtp : *promptData)
		{
			delete rtp;
		}
		_logger->info("Prompt::~Prompt", "Packets removed for prompt ");
	}
	catch (std::exception& e)
	{
		_logger->error("Prompt::~Prompt", " Exception ... " + string(e.what()));
	}
}

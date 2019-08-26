#include "Load.h"


bool Load::makePackets(int promptID, std::string promptName) {

	try {
		promptName.assign("/sounds/" + promptName + ".");
		std::vector<rtp_prompt*>* promptData = new std::vector<rtp_prompt*>();

		for (auto &itr : Configurations::_CodecTypesMap)
		{			
			rtp_prompt *rtp = new rtp_prompt();
			if (!rtp->load(promptName + itr.first, itr.second)) {
				_logger->error("Load::makePackets", "Could not make RTP packets for Prompt: " + std::to_string(promptID) + " CODEC: " + itr.first);
				return false;
			}
			promptData->push_back(rtp);
//			_logger->info("Load::makePackets", "For Prompt: " + std::to_string(promptID) + " CODEC: " + itr.first);

		}	
		Prompt* promptObj = new Prompt(promptID, promptData);
		MediaServer::promptsMapLock.lock();
		MediaServer::promptsMap->insert(std::make_pair(promptID, promptObj));
		MediaServer::promptsMapLock.unlock();
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Load::makePacket"," error while creating packet for... " + std::to_string(promptID) + string(ex.__cxa_exception_type()->name()));
		return false;
	}
}

bool Load::loadprompts()
{
	try
	{
		_logger->info("Load::loadprompts", "Loading Prompt from configurations  .....");
		for (auto &itr : Configurations::_PromptsMap)
		{
			if (!makePackets(itr.first, itr.second))
			{
				_logger->error("Load::loadprompts", "Error while Loading Prompt " + std::to_string(itr.first) + " : " + itr.second);
			}
		}
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Load::loadprompts", " Exception " + string(ex.__cxa_exception_type()->name()));
		return false;
	}	
}
void Load::unloadPrompt(int promptID)
{
	try
	{
		_logger->info("Load::unloadPrompt", "Unloading prompt: " + std::to_string(promptID));
		MediaServer::promptsMapLock.lock();
		Prompt* packetVect = MediaServer::promptsMap->at(promptID);
		MediaServer::promptsMap->erase(promptID);
		MediaServer::promptsMapLock.unlock();
		delete packetVect;
	//	_logger->info("Load::unloadPrompt", "successfully Unloaded prompt: " + std::to_string(promptID));		
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Load::unloadPrompt", " Exception " + string(ex.__cxa_exception_type()->name()));
	}
}
Load::Load()
{
	_logger = GlobalLogger::StaticLogger;
}


Load::~Load()
{
}

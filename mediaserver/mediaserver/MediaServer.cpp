#include "MediaServer.h"

std::map<int, Prompt*>* MediaServer::promptsMap  = new std::map<int, Prompt*>();
std::mutex MediaServer::promptsMapLock;
BlockingQueue<Common::playMessage*>* MediaServer::_mainPromptQueue = new BlockingQueue<Common::playMessage*>();
BlockingQueue<Common::playMessage *>* MediaServer::_stopPromptQueue = new BlockingQueue<Common::playMessage*>();
BlockingQueue<Common::Message*>* MediaServer::_processClientMessageQueue = new BlockingQueue<Common::Message*>();
BlockingQueue<Common::Message*> MediaServer::_responseMsgToClient;
BlockingQueue<int> MediaServer:: _processUnloadMessageQueue;
PlayPrompt * play;


bool MediaServer::start() {
	try
	{
		Load load;
		_logger->info("Loading CODECs");
		if (!load.loadprompts())
		{
			_logger->warn("Error loading prompts, exiting...");
			return false;
		}
		play->startPlayPromptThread();
		thread* clientMsgProcessorThreadPtr = new std::thread(&MediaServer::ClientMsgProcessorThread, this);
		thread* UnloadMsgProcessorThreadPtr = new std::thread(&MediaServer::ProcessUnloadMessageThread, this);
		thread* SendRespToClientThreadPtr = new std::thread(&MediaServer::SendRespToClientThread, this);
		
		Common::threadVect->push_back(clientMsgProcessorThreadPtr);
		Common::threadVect->push_back(UnloadMsgProcessorThreadPtr);
		Common::threadVect->push_back(SendRespToClientThreadPtr);		
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::StopPromptThread", " Exception: " + string(ex.__cxa_exception_type()->name()));
		return false; 
	}
}


bool MediaServer::addCodec(std::string promptID, int payLoadType) {
	bool status = true;
	if (!Configurations::_CodecTypesMap.insert(std::make_pair(promptID, payLoadType)).second)
	{
		_logger->error("MediaServer::addCodec","Error in adding codec " + promptID);
		status = false;
	}
	return status;
}

bool MediaServer::stopPrompt(Common::defaultMessage* stopMsg)
{
	std::string callID;
	try
	{
		std::map<std::string, Common::playMessage *>::iterator itr;
		bool status = false;
		callID = stopMsg->msg;

		PlayPrompt::_callsMapLock->lock();		
		itr = PlayPrompt::callsMap->find(callID);
		if (itr != PlayPrompt::callsMap->end())
		{
			itr->second->stop = true;
			PlayPrompt::callsMap->erase(itr);
			status = true;
		}			
		PlayPrompt::_callsMapLock->unlock();
		_responseMsgToClient.Enqueue(stopMsg);
//		_logger->info("MediaServer::stopPrompt", to_string(PlayPrompt::callsMap->size()) + " Removing callID: " + callID);
		return status;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::stopPrompt", " Exception " + callID + string(ex.__cxa_exception_type()->name()));
		return false;
	}
}

void MediaServer::ProcessPlayMessage(Common::Message* clientMsg)
{
	try
	{
		std::map<int, Prompt*>::iterator itr;
		Common::playMessage* messageObject = (Common::playMessage*)clientMsg;
		PlayPrompt::_callsMapLock->lock();
		PlayPrompt::callsMap->insert(std::pair<std::string, Common::playMessage *>(messageObject->callID, messageObject));
		PlayPrompt::_callsMapLock->unlock();
		itr = MediaServer::promptsMap->find(messageObject->promptID);
		if (itr == MediaServer::promptsMap->end() || !itr->second->incrementRefCount())
		{
			messageObject->response = Common::RESP_PROMPT_NOT_FOUND_ERR;
			_responseMsgToClient.Enqueue(messageObject);
			_logger->error("MediaServer::ProcessPlayMessage", "Requested Prompt is not available in the list");
		}
		else
		{
			MediaServer::_mainPromptQueue->Enqueue(messageObject);
			//		_logger->info("MediaServer::ProcessPlayMessage", "Message Enqued for callID: " + callID);					//uncomment		
		}		
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::ProcessPlayMessage", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}
	
}


void MediaServer::ClientMsgProcessorThread()
{
	try
	{
		Load load;
		Common::Message * message = NULL;
		while (Common::keepRunning)
		{
			
			MediaServer::_processClientMessageQueue->Dequeue(message);
			if (message)
			{
				if (message->msgType == 1)
				{
					ProcessPlayMessage(message);
				}
				else if (message->msgType == 2)
				{
					Common::defaultMessage* stopMsg = ((Common::defaultMessage*)message);
					std::string callID = stopMsg->msg;
					if (!stopPrompt(stopMsg))
						_logger->error("MediaServer::ClientMsgProcessorThread", "StopPrompt: callID: " + callID + " not available in callsMap ");
					//	delete stopMsg;
				}
				else if (message->msgType == 3)
				{
					int ID = std::stoi(((Common::defaultMessage*)message)->msg);
					std::map<int, Prompt*>::iterator itr = promptsMap->find(ID);

					if (itr != promptsMap->end())
					{
						if (itr->second->getRefCount() == 0)
						{
							itr->second->setStopAssigning(true);
							_processUnloadMessageQueue.Enqueue(ID);
						}
						else
						{
							itr->second->remove = true;
						}
					}
					delete message;
				}
				else
				{
					_logger->error("MediaServer::ClientMsgProcessorThread", "Wrong message type from Client= " + std::to_string(message->msgType));
				}
				message = NULL;
			}			
		}
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::ClientMsgProcessorThread", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}	
}

void MediaServer::ProcessUnloadMessageThread()
{
	Load load;
	try
	{
		_logger->info("Initializing ProcessUnloadMessageThread");
		while (Common::keepRunning)
		{
			int PromptID;
			_processUnloadMessageQueue.Dequeue(PromptID);
			if (!Common::keepRunning)
				break;
			unloadPrompt(PromptID);
		}
		_logger->info("closing ProcessUnloadMessageThread");
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::ProcessUnloadMessageThread", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}
	
}

void MediaServer::unloadPrompt(int promptID)
{
	try
	{
		_logger->info("MediaServer::unloadPrompt", "Unloading prompt: " + std::to_string(promptID));
		MediaServer::promptsMapLock.lock();
		Prompt* packetVect = MediaServer::promptsMap->at(promptID);
		MediaServer::promptsMap->erase(promptID);
		MediaServer::promptsMapLock.unlock();
		delete packetVect;
		_logger->info("MediaServer::unloadPrompt", "successfully Unloaded prompt: " + std::to_string(promptID));
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::unloadPrompt", " Exception " + string(ex.__cxa_exception_type()->name()));
	}
}

void MediaServer::SendRespToClientThread()
{
	try {
		Common::Message* queueMessage;
		Common::playMessage* playMessage;
		Common::defaultMessage* stopMessage;
		_logger->info("MediaServer::SendRespToClientThread", "Starting SendRespToClientThread ");
		while (Common::keepRunning)
		{
			_responseMsgToClient.Dequeue(queueMessage);
			if (queueMessage) {
				if (queueMessage->msgType == 1)
				{
					playMessage = (Common::playMessage*)queueMessage;
					if (MediaServer::promptsMap->find(playMessage->promptID)->second->decrementRefCount())
						MediaServer::_processUnloadMessageQueue.Enqueue(playMessage->promptID);
					if (playMessage->response == Common::RESP_MSG_STOPPED)
					{
						delete playMessage;
						continue;
					}
					sendMessage(playMessage->socketID, playMessage->callID, playMessage->response);
					playMessage->remove = true;
					delete playMessage;
				}
				else
				{
					stopMessage = (Common::defaultMessage*)queueMessage;
					sendMessage(stopMessage->socketID, stopMessage->msg, Common::RESP_MSG_STOPPED);
					delete stopMessage;
				}
				queueMessage = NULL;
			}
						
		}
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::ProcessUnloadMessageThread", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}
	
}


bool MediaServer::sendMessage(int socketID,char msg[20], Common::ENResponseCode RespCode)
{
	Common::ResponseMessage respMsg;
	int msgType = 1;
	try {
		memcpy(respMsg.callID, msg, 20);
		respMsg.RespCode = RespCode;

		if (send(socketID, &msgType, 4, 0) < 0)
		{
			_logger->error("MediaServer::sendMessage", "Error in sending message type to media server");
		}

		if (send(socketID, &(respMsg), sizeof(respMsg), 0) < 0)
		{
			_logger->error("MediaServer::sendMessage", "Error in sending message type to media server");
		}
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("MediaServer::sendMessage", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}
	
}

bool MediaServer::stopMediaServer()
{
	try {		
		_mainPromptQueue->Stop();
		_stopPromptQueue->Stop();
		_processClientMessageQueue->Stop();
		_responseMsgToClient.Stop();
		_processUnloadMessageQueue.Stop();
		play->stopPlayPrompt();

		for (auto itr = MediaServer::promptsMap->begin(), next_it = itr; itr != MediaServer::promptsMap->end(); itr = next_it)
		{
			++next_it;
			unloadPrompt(itr->first);

		}
		delete promptsMap;
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::stopMediaServer", " Exception: " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
}

void MediaServer::shutDown()
{
	try
	{
	int threadVectsize = Common::threadVect->size();
	int socketIDVectsize = Common::socketIDVect->size();
	stopMediaServer();
	for (int i = 0; i <socketIDVectsize; i++)
	{
		close(Common::socketIDVect->at(i));
	}
	for (int i = 0; i <threadVectsize; i++)
	{
		std::thread* threadPtr = Common::threadVect->at(i);
		threadPtr->detach();
		delete threadPtr;
	}
	Common::threadVect->erase(Common::threadVect->begin(), Common::threadVect->end());
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		std::cerr << "Error deleting thread vector";
	}
}

MediaServer::MediaServer()
{
	_logger = GlobalLogger::StaticLogger;
	play = new PlayPrompt();
}


MediaServer::~MediaServer()
{		
	delete play;
	delete _mainPromptQueue;
	delete _stopPromptQueue;
}

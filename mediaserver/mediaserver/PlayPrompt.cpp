#include "PlayPrompt.h"
#include <list>

std::vector<rtp_target> rtp_targets[MAX_PLAY_PROMPT_THREADS];
vector<thread*>* PlayPrompt::tid = new vector<thread*>();
std::map<std::string, Common::playMessage *>* PlayPrompt::callsMap = new std::map<std::string, Common::playMessage *>();
std::mutex* PlayPrompt::_callsMapLock = new std::mutex();


bool PlayPrompt::playMessagesThread(int threadIndex)
{
	struct timespec start, stop;
	int packetIndex, freePortsQueueInd = threadIndex * 1000;
	std::list<int> availablePorts;
	PromptPort* portsArr = messagePortsArr[threadIndex];
	try {

		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (-1 == sockfd)
		{
			_logger->error("PlayPrompt::playMessages", "Failed to create socket");
			return false;
		}
		_logger->info("PlayPrompt::playMessages", "playMessages Thread Initialized " + to_string(threadIndex));
	
		while (Common::keepRunning)
		{
			clock_gettime(CLOCK_MONOTONIC, &start);
			for(int index = 0; index<MAX_PPORTS_ER_THREAD; index++)
			{
				if (!portsArr[index].free)
				{			
					std::string callID = portsArr[index].messageDetail->callID;
					if (portsArr[index].messageDetail->stop)
					{	
						clearPort(&(portsArr[index]), Common::RESP_MSG_STOPPED);
						threadFreePortsCount->Enqueue(index + freePortsQueueInd);
						_logger->info("PlayPrompt::playMessages", "Message stopped by IVR for CallID: " + callID);						
						continue;
					}
					packetIndex = portsArr[index].rtpTarget->sendnextpacket(sockfd);
					if (packetIndex == -1)
						_logger->error("Error in sending packet.Thread Index:" + std::to_string(threadIndex));

					if (checkStopConditions(&(portsArr[index]), &packetIndex))
					{
						clearPort(&(portsArr[index]), Common::RESP_MSG_COMPLETE);
						threadFreePortsCount->Enqueue(index + freePortsQueueInd);
						_logger->info("PlayPrompt::playMessages", "Message Condition ended for CallID: " + callID);
						continue;
					}	
				}
			}
				clock_gettime(CLOCK_MONOTONIC, &stop);
				addPayLoadDelay(&start, &stop, threadIndex);
		}
		_logger->info("PlayPrompt::playMessages", "Terminating playMessagesThread# " + to_string(threadIndex));
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::playMessages", " Exception ... " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
}

bool PlayPrompt::checkStopConditions(PromptPort* promptPort, int* packetIndex)
{
	bool status = false;

	if (promptPort->messageDetail->unlimited)
		status = false;
	else if ((promptPort->messageDetail->Timeout > 0) && (promptPort->messageDetail->Timeout <= (Timer::currentTime - promptPort->startTime)))
		status = true;
	else if (*packetIndex == promptPort->packetCount)
	{
		promptPort->messageDetail->loop--;
		if (promptPort->messageDetail->loop == 0)
			status = true;
	}
	return status;
}

bool PlayPrompt::clearPort(PromptPort* promptPort, Common::ENResponseCode response)
{
	try {
		promptPort->messageDetail->response = response;
		promptPort->resetPort();
		MediaServer::_stopPromptQueue->Enqueue(promptPort->messageDetail);
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::clearPort", " Exception ... " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
}

void PlayPrompt::addPayLoadDelay(struct timespec* start, struct timespec* stop, int threadIndex)
{
	struct timespec diff;

	if ((stop->tv_nsec - start->tv_nsec) < 0)
	{
		diff.tv_sec = stop->tv_sec - start->tv_sec - 1;
		diff.tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	}
	else
	{
		diff.tv_sec = stop->tv_sec - start->tv_sec;
		diff.tv_nsec = stop->tv_nsec - start->tv_nsec;
	}
	if (diff.tv_sec > 0 || diff.tv_nsec > PayLoadSize)
	{
		_logger->error("PlayPrompt::playMessagesThread", "threadFreePortsCount= "+to_string(threadFreePortsCount->getSize())+ " MediaServer::_mainPromptQueue= " + to_string(MediaServer::_mainPromptQueue->getSize()) +"Thread: " + to_string(threadIndex) + " Processing overload. " + " time= " + to_string(diff.tv_sec) + "." + to_string(diff.tv_nsec));
	}
	else
	{
		__useconds_t sleepTime = (__useconds_t)(PayLoadSize - diff.tv_nsec) / 1000;
		usleep(sleepTime);
	}
}

void PlayPrompt::startPlayPromptThread()
{
	try {
		if (tid->size() <= MAX_PLAY_PROMPT_THREADS)
		{

			for (int threadIndex = 0; threadIndex < MAX_PLAY_PROMPT_THREADS; threadIndex++)
			{
				for (int portIndex = 0; portIndex < MAX_PPORTS_ER_THREAD; portIndex++)
				{
					threadFreePortsCount->Enqueue(portIndex + (threadIndex * 1000));
					messagePortsArr[threadIndex][portIndex].free = true;
				}

				thread* ptrplayPromptThrea = new std::thread(&PlayPrompt::playMessagesThread, this, threadIndex);
				pthread_setschedprio(ptrplayPromptThrea->native_handle(), 2);

				if (!ptrplayPromptThrea)
				{
					_logger->error("Error Creating Thread Index:" + std::to_string(0) + " Error:" /*+ std::to_string(thread_ret)*/);
					break;
				}
				Common::threadVect->push_back(ptrplayPromptThrea);
//				playPromptQueueMap->push_back(_playPromptThreaQueue);
			}
	
		}
	
		
		thread* ptrStopPromptThread = new std::thread(&PlayPrompt::StopPromptThread, this);
		ptrloaBalancerThread = new std::thread(&PlayPrompt::loadBalancerThread, this);
		if (!ptrloaBalancerThread)
				{
					_logger->error("PlayPrompt::startPlayPromptThread","Error Creating ptrloaBalancerThread");
				}

		Common::threadVect->push_back(ptrStopPromptThread);
		Common::threadVect->push_back(ptrloaBalancerThread);
		_logger->info("PlayPrompt::play", "PlayPromptThread...");
		//usleep(500000);
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::startPlayPromptThread", " Error while creating packet for... " + string(ex.__cxa_exception_type()->name()));
	}
}

void PlayPrompt::StopPromptThread()
{
	try{
		std::map<std::string, Common::playMessage*>::iterator itr;
		Common::playMessage* clientMessageObj = NULL;
		_logger->info("PlayPrompt::StopPromptThread", "StopPromptThread Started ...");
		while (Common::keepRunning)
		{
			MediaServer::_stopPromptQueue->Dequeue(clientMessageObj);
			if (clientMessageObj)
			{
				std::string callID = clientMessageObj->callID;
				if (!clientMessageObj->stop)
				{
					_callsMapLock->lock();
					callsMap->erase(callID);
					_callsMapLock->unlock();
					_logger->info("PlayPrompt::StopPromptThread", to_string(callsMap->size()) + "  Completed Succesfully. Removing callID: " + callID);
				}
				MediaServer::_responseMsgToClient.Enqueue(clientMessageObj);
				clientMessageObj = NULL;
			}
			
		}
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::StopPromptThread", " Exception " + string(ex.__cxa_exception_type()->name()));
	}
}


void PlayPrompt::loadBalancerThread()
{
	try {
		int count = 0, threadIndex = 0, portIndex = 0, index;
		Common::playMessage * message = NULL;
		_logger->info("PlayPrompt::loadBalancerThread", "LoadBalancer Thread Initialized");

		while (Common::keepRunning)
		{					
			threadFreePortsCount->Dequeue(index);
			MediaServer::_mainPromptQueue->Dequeue(message);
			if (message)
			{
				threadIndex = index / 1000;
				portIndex = index % 1000;

				//			_logger->info("PlayPrompt::loadBalancerThread", "Assigning to [" + to_string(threadIndex) + "][" + to_string(portIndex));

				if (!messagePortsArr[threadIndex][portIndex].setPort(message))
				{
					messagePortsArr[threadIndex][portIndex].free = true;
					_logger->error("PlayPrompt::loadBalancerThread", "Error in setting port [" + to_string(threadIndex) + "][" + to_string(portIndex));
				}
				//		playPromptQueueMap->at(finalIndex)->Enqueue(message);
				count++;
				message = NULL;
			}			
		}
		_logger->info("PlayPrompt::loadBalancerThread", "Closing loadBalancerThread");
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("PlayPrompt::loadBalancerThread", " error while creating packet for... " + string(ex.__cxa_exception_type()->name()));
	}
}

int PlayPrompt::findMinLoadQueue()
{
	int min, count, finalIndex = 0;

	min = UINT16_MAX;

	//for (BlockingQueue<Common::playMessage*> *threadQueue : *playPromptQueueMap)
	for (int ind = 0; ind<MAX_PLAY_PROMPT_THREADS; ind++)
	{
		count = threadFreePortsCount->getSize();
		if (count < min)
		{
			min = count;
			finalIndex = ind;
		}
	}
	return finalIndex;
}


PlayPrompt::PlayPrompt()
{
	_logger = GlobalLogger::StaticLogger;
	//MediaServer::_stopPromptQueue = new BlockingQueue<PlayPromptParamsMultiple*>();
//	playPromptQueueMap = new std::vector<BlockingQueue<Common::playMessage*>*>();

	threadFreePortsCount = new BlockingQueue<int>();
	messagePortsArr = new PromptPort*[MAX_PLAY_PROMPT_THREADS];
	for (int i = 0; i < MAX_PLAY_PROMPT_THREADS; i++)
		messagePortsArr[i] = new PromptPort[MAX_PPORTS_ER_THREAD];
}

bool PlayPrompt::stopPlayPrompt()
{
	threadFreePortsCount->Stop();
}


PlayPrompt::~PlayPrompt()
{
	_logger->error("PlayPrompt::PlayPrompt", "In Destructor");
	int size = playPromptPtrVect.size();


	for (int i = 0; i < MAX_PLAY_PROMPT_THREADS; i++)
		delete[] messagePortsArr[i];
	delete[] messagePortsArr;
//	delete playPromptQueueMap;
	delete callsMap;
	delete _callsMapLock;
	delete threadFreePortsCount;
	delete tid;
}



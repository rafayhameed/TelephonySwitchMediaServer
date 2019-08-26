#pragma once

#include "rtp_target.h"
#include "Common.h"
#include "MediaServer.h"
#include "BlockingQueue.h"
#include "Timer.h"
#include "PromptPort.h"
class PromptPort;

class PlayPrompt
{

private:
	Logger * _logger = NULL;
	bool bThreadExit = false;
//	std::vector<BlockingQueue<Common::playMessage*>*> *playPromptQueueMap;
	thread* ptrloaBalancerThread;
	std::vector<thread*> playPromptPtrVect;
	BlockingQueue<int>* threadFreePortsCount;
	PromptPort** messagePortsArr;
public:
	PlayPrompt();
	~PlayPrompt();
	void startPlayPromptThread();
	static vector<thread*>* tid;
	static std::map<std::string, Common::playMessage *> *callsMap;
	static std::mutex* _callsMapLock;
	void loadBalancerThread();
	bool playMessagesThread(int threadIndex);
	void addPayLoadDelay(struct timespec* start, struct timespec* stop, int threadIndex);
	int findMinLoadQueue();
	void StopPromptThread();
	bool checkStopConditions(PromptPort* promptPort, int* packetIndex);
	bool clearPort(PromptPort* promptPort, Common::ENResponseCode response);
	bool stopPlayPrompt();
};


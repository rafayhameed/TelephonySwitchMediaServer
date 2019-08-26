#pragma once
#include "Common.h"
#include <atomic>
#include "rtp_prompt.h"



class Prompt
{
private:
	std::mutex promptLock;
	std::atomic<int> refCount;
	Logger * _logger = NULL;
	std::vector<rtp_prompt*>* promptData;
	bool stopAssinging = false;
public:
	int promptID = 0;
	bool remove = false;
	std::vector<rtp_prompt*>* getPromptPackets();
	void setPromptPackets(std::vector<rtp_prompt*>* promptObj);
	int getRefCount();
	bool decrementRefCount();
	bool incrementRefCount();
	bool getStopAssigning();
	void setStopAssigning(bool stop);
	Prompt(int ID, std::vector<rtp_prompt*>* promptObj);
	~Prompt();
};


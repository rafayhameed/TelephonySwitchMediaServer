#pragma once
#include"Common.h"
#include"rtp_prompt.h"
#include"rtp_target.h"
#include"Load.h"
#include"PlayPrompt.h"
#include"BlockingQueue.h"
#include"Prompt.h"

class MediaServer
{

private:
	Logger * _logger = NULL;
public:
	static std::mutex promptsMapLock;
	static std::map<int , Prompt*>* promptsMap;
	static BlockingQueue<Common::playMessage*>* _mainPromptQueue;
	static BlockingQueue<Common::playMessage*>* _stopPromptQueue;
	static BlockingQueue<Common::Message*>* _processClientMessageQueue;
	static BlockingQueue<int> _processUnloadMessageQueue;
	static BlockingQueue<Common::Message*> _responseMsgToClient;


	void ClientMsgProcessorThread();
	bool addCodec(std::string promptID, int payLoadType);
	bool start();
	void unloadPrompt(int promptID);
	bool stopPrompt(Common::defaultMessage* stopMsg);
	void ProcessPlayMessage(Common::Message* msg);
	void ProcessUnloadMessageThread();
	void SendRespToClientThread();
	bool sendMessage(int socketID, char msg[20], Common::ENResponseCode RespCode);
	bool stopMediaServer();
	void shutDown();
	MediaServer();
	~MediaServer();
};


#pragma once
#include <cstdio>
#include <sys/types.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <map>
#include"AfnLogger.h"
#include"BlockingQueue.h"




class GlobalLogger
{
public:
	static Logger * StaticLogger;	
};

void trim(std::string& str);
void rtrim(std::string& str);
void ltrim(std::string& str);

class Common
{

public:
#define MIN_PROMPT_FILE_SIZE		1024
#define MAX_PROMPTS					10
#define RTP_TRAGETS_PER_THREAD		1
#define MAX_PLAY_PROMPT_THREADS		4
#define MAX_PPORTS_ER_THREAD		1000
#define PayLoadSize					20000000
#define TimerSleep					1
#define RecvPlayMsgSize				80
#define RecvStopMsgSize				28
#define PlayMsgSize					88
#define StopMsgSize					28
//#define RTP_TRAGETS_PER_THREAD		1/*1125*/
//#define MAX_PLAY_PROMPT_THREADS		1/*4*/

	enum ENResponseCode
	{
		RESP_SUCCESS = 00,
		RESP_FAILED = 01,
		RESP_MSG_FORMAT_ERROR = 02,
		RESP_MSG_COMPLETE = 03,
		RESP_MSG_STOPPED = 04,
		RESP_PROMPT_NOT_FOUND_ERR = 05
	};

	struct Message
	{
		int msgType;
		int socketID = 0;
	};

	struct playMessage : Message
	{
		char callID[20];
		int promptID = 0;
		int loop = 0;
		char codecType[10];
		int Timeout = 0;
		bool unlimited = false;
		char IP[20];
		unsigned short port = 0;
		bool last = false;
		bool stop = false;
		ENResponseCode response;
		bool remove = false;
	};

	struct promptPortParams
	{
		playMessage* message;
		bool stop;
	};

	struct defaultMessage : Message
	{
		char msg[20];
	};

	struct rtp_hdr_t
	{
		//little endian only at the moment
		unsigned int cc : 4;       /* CSRC count */
		unsigned int x : 1;        /* header extension flag */
		unsigned int p : 1;        /* padding flag */
		unsigned int version : 2;  /* protocol version */
		unsigned int pt : 7;       /* payload type */
		unsigned int m : 1;        /* marker bit */
		unsigned short seq;			/* sequence number */
		unsigned int ts;            /* timestamp */
		unsigned int ssrc;          /* synchronization source */
	};

	struct rtp_packet_t
	{
		rtp_hdr_t hdr;
		unsigned short data[80];
	};

	struct stopPromptMessage
	{
		char callID[20];
		ENResponseCode RespCode;
		int promptID = 0;
		stopPromptMessage(std::string callid, ENResponseCode Code, int ID)
		{
			strncpy(callID, callid.c_str(), callid.length());
			callID[callid.length()] = '\0';
			RespCode = Code;
			promptID = ID;
		}
	};

	struct ResponseMessage
	{
		char callID[20];
		ENResponseCode RespCode;
	};

	struct ResponseMessageParams
	{
		stopPromptMessage* message;
		int socketID;
	};


	struct ClientMessage
	{
		int socketID;
		Message* message;
		bool stop = false;
		ENResponseCode response = RESP_SUCCESS;
		std::string callID;
	};

	struct CallRequest
	{
		char callID[20];
		long DNIS;
	};

	static int IVRSocketID;
	static std::vector<thread*>* threadVect;
	static std::vector<int>* socketIDVect;
	
	static bool keepRunning;

	Common();
	~Common();
};


#pragma once

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <future>
#include "Common.h"
#include "MediaServer.h"

class ClientListener
{
private:
	Logger * _logger = NULL;
	int sockfd, newsockfd, port;
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	thread serverThread;
	bool stopThread = false;

public:
	void startClientListener(std::string IP, int port);
	std::string acceptClient();
	void* RecvFromClient();
	bool SendToClient();


	ClientListener();
	~ClientListener();




};


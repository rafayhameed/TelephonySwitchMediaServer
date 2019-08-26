#include "ClientListener.h"
#include "LoadApp.h"

std::vector<std::future<void*>> futures;
bool check = false;

void ClientListener::startClientListener(std::string IP, int port)
{
	_logger->info("ClientListener::startClientListener", "Starting Client Listener");

	try
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = inet_addr(IP.c_str());
		serverAddress.sin_port = htons(port);
		if (bind(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		{
			std::cout << "Error in Listen" << std::endl;
		}
		if (listen(sockfd, SOMAXCONN) < 0)		//   SOMAXCONN  =  maximum possible value for accept queue
		{
			std::cout << "Error in Listen" << std::endl;
		}
		_logger->info("ClientListener::startClientListener", "Started Listening at " + IP + " : " + to_string(port));

		thread* ptrAcceptClientThread = new std::thread(&ClientListener::acceptClient, this);
		Common::threadVect->push_back(ptrAcceptClientThread);
		Common::socketIDVect->push_back(sockfd);
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("ClientListener::startClientListener", " error while creating Listen Socket... " + string(ex.__cxa_exception_type()->name()));
	}
}

std::string ClientListener::acceptClient()
{
	string addrs;
	try
	{
		LoadApp app;
		while (Common::keepRunning)
		{
			socklen_t sosize = sizeof(clientAddress);
			newsockfd = accept(sockfd, (struct sockaddr*)&clientAddress, &sosize);
			if (newsockfd < 0)
				break;

			addrs = inet_ntoa(clientAddress.sin_addr);
			_logger->info("ClientListener::acceptClient", "Client requested for connection " + addrs + " : " + to_string(clientAddress.sin_port));
			Common::IVRSocketID = newsockfd;

			thread* ptrthreadhandlenew = new std::thread(&ClientListener::RecvFromClient, this);
			Common::threadVect->push_back(ptrthreadhandlenew);
			Common::socketIDVect->push_back(newsockfd);

			std::cout << "Starting Load Test App" << std::endl;
			app.StartLoadApp();
		}
		return addrs;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("ClientListener::acceptClient", "Exception "+ string(ex.__cxa_exception_type()->name()));
		return NULL;
	}
}


void* ClientListener::RecvFromClient()
{
	int recvdBytes;
	int rmngBytesPlay = RecvPlayMsgSize;
	int rmngBytesStop = RecvStopMsgSize;
	int msgType;
	char *msgBuff;
	int startingPoint = 0;
	try
	{
		std::cout << "starting RecvFromClient thread" << std::endl;
		while (!stopThread)
		{
			recvdBytes = recv(newsockfd, &msgType, 4, 0);
			if (0 == recvdBytes)
			{
				std::cerr << "Connection closed. Handle:" << newsockfd << std::endl;
				_logger->error("ClientListener::RecvFromClient", "Connection closed. Handle:" + to_string(newsockfd));
				close(newsockfd);
				stopThread = true;
				break;
			}
			recvdBytes = 0;
			startingPoint = 0;
			if (1 == msgType)
			{
				msgBuff = new char[PlayMsgSize];
				while (recvdBytes<RecvPlayMsgSize)
				{
					recvdBytes = recv(newsockfd, msgBuff + startingPoint, rmngBytesPlay, 0);
					if (0 == recvdBytes)
					{
						std::cerr << "Connection closed. Handle:" << newsockfd << std::endl;
						_logger->error("ClientListener::RecvFromClient", "Connection closed. Handle:" + to_string(newsockfd));
						close(newsockfd);
						stopThread = true;
						break;
					}
					if (recvdBytes<rmngBytesPlay)
					{
						rmngBytesPlay = RecvPlayMsgSize - recvdBytes;
						startingPoint = recvdBytes;
						continue;
					}
					rmngBytesPlay = RecvPlayMsgSize;
					recvdBytes = RecvPlayMsgSize;
				}
			}
			else
			{
				msgBuff = new char[StopMsgSize];
				while (recvdBytes<RecvStopMsgSize)
				{
					recvdBytes = recv(newsockfd, msgBuff, rmngBytesStop, 0);
					if (0 == recvdBytes)
					{
						std::cerr << "Connection closed. Handle:" << newsockfd << std::endl;
						_logger->error("ClientListener::RecvFromClient", "Connection closed. Handle:" + to_string(newsockfd));
						close(newsockfd);
						stopThread = true;
						break;
					}
					if (recvdBytes<rmngBytesStop)
					{
						rmngBytesStop = RecvStopMsgSize - recvdBytes;
						startingPoint = recvdBytes;
						continue;
					}
					rmngBytesStop = RecvStopMsgSize;
					recvdBytes = RecvStopMsgSize;
				}
			}
			if (!stopThread)
			{
				((Common::Message*)msgBuff)->socketID = newsockfd;
				MediaServer::_processClientMessageQueue->Enqueue((Common::Message*)msgBuff);		
			}
		}
		_logger->error("ClientListener::RecvFromClient", "Terminating Thread: " + to_string(newsockfd));
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("ClientListener::RecvFromClient", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}
	return 0;
}

bool ClientListener::SendToClient()
{
	Common::ENResponseCode message;
	try
	{
		if (send(newsockfd, &message, sizeof(message), 0) < 0)
		{
			_logger->error("TCPClient::Send", "Error in sending message type to media server");
			return false;
		}
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("ClientListener::SendToClient", " Exception: " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
}


ClientListener::ClientListener()
{
	_logger = GlobalLogger::StaticLogger;
}


ClientListener::~ClientListener()
{
	
}

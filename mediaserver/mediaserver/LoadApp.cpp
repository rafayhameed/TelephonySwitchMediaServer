#include "LoadApp.h"



void LoadApp::StartLoadApp()
{
	_logger->info("LoadApp::StartLoadApp", "Starting Load Testing app");

	thread* loadTestThread = new std::thread(&LoadApp::LoadTestingThread, this);
	Common::threadVect->push_back(loadTestThread);
	if(!loadTestThread)
		_logger->error("LoadApp::StartLoadApp", "LoadTestingThread Failed");
	
}





void LoadApp::LoadTestingThread()
{
	_logger->info("LoadApp::LoadTestingThread", "LoadTestingThread started");
	int index = 0;
	long DNIS = 123416;
	int msgType = 2;
	while (Common::keepRunning)
	{
		std::string callIDIndex;
		Common::CallRequest message;

		for (int i = 0;i<6;i++)
		{

			callIDIndex = "CALL" + to_string(index++);
			DNIS = DNIS + 10;


			strncpy(message.callID, callIDIndex.c_str(), callIDIndex.length());
			message.DNIS = DNIS + 10;;

			if (send(Common::IVRSocketID, &msgType, 4, 0) < 0)
			{
				_logger->error("LoadApp::LoadTestingThread", "Error in sending message type call request to media server");
			}
			
			if (send(Common::IVRSocketID, &(message), sizeof(message), 0) < 0)
			{
				_logger->error("LoadApp::LoadTestingThread", "Error in sending message Call Request to media server");
			}
		}
		this_thread::sleep_for(std::chrono::seconds(1));
		int g;
		std::cin >> g;
	}

	

}

LoadApp::LoadApp()
{
	_logger = AfnLogger::getInstance("/tmp/LoadTestAppLogs/");
}


LoadApp::~LoadApp()
{
}

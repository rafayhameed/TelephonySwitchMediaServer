#include<iostream>
#include <vector>
#include"MediaServer.h"
#include"Common.h"
#include"Timer.h"
#include"Configurations.h"
#include"ClientListener.h"
#include"LoadApp.h"


int main()
{
	GlobalLogger::StaticLogger = AfnLogger::getInstance("/tmp/MediaServerLogs/");
	GlobalLogger::StaticLogger->info("starting application");
	Common::threadVect = new std::vector<thread*>();
	Common::socketIDVect = new std::vector<int>();
	Timer time;
	Configurations configs;
	MediaServer mediaServer;
	LoadApp loadTestObj;
	
	std::cout << "************************************" << std::endl;
	std::cout << "******  Afiniti Media Server  ******" << std::endl;
	std::cout << "************************************" << std::endl;

	configs.ReadConfigurations();
	time.startTimer();

	if (!mediaServer.start())
		std::cout << "Error While Starting Media Server" << std::endl;

	

	ClientListener clientListener;
	clientListener.startClientListener("10.105.27.99", 2000);
	






	std::string input;
	std::cout << "Enter --- shutdown --- to Close Media Server" << std::endl;
	while (input != "shutdown")
	{
		std::cout << "Enter Command....... " << std::endl;
		std::cin >> input;
	}
	
		Common::keepRunning = false;	
		this_thread::sleep_for(std::chrono::seconds(2));
		mediaServer.shutDown();	

	if (!AfnLogger::closeLogger("/tmp/MediaServerLogs/")) {
		std::cout << "Error while closing log" << std::endl;
	}
	return 0;
}
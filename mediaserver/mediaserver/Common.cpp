#include "Common.h"

Logger * GlobalLogger::StaticLogger = NULL;
std::vector<thread*>* Common::threadVect;
std::vector<int>* Common::socketIDVect;
int Common::IVRSocketID = 0;
bool Common::keepRunning = true;

void ltrim(std::string &str)
{
	str.erase(0, str.find_first_not_of("\t\n\v\f\r "));
}

void rtrim(std::string &str)
{
	str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1);
}

void trim(std::string &str)
{
	rtrim(str);
	ltrim(str);
}


Common::Common()
{
}


Common::~Common()
{
}

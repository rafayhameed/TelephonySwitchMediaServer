#include "Configurations.h"

std::map<std::string, std::string> Configurations::_configurations;
std::map<std::string, int> Configurations::_CodecTypesMap;
std::map<int, std::string> Configurations::_PromptsMap;

void Configurations::ReadConfigurations()
{
	try
	{
		_logger->info("Configurations::ReadConfigurations", "Reading Configurations ........");
		if (!ReadCodecsFile())
			_logger->error("Configurations::ReadConfigurations", "Error while reading  Codecs File");

		if (!ReadPromptsFile())
			_logger->error("Configurations::ReadConfigurations", "Error while reading  Prompts File");
		_logger->info("Configurations::ReadConfigurations", "Configurations Loaded successfully. Total Loaded Prompts " + std::to_string(_PromptsMap.size()));
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Configurations::ReadConfigurations", " Exception: " + string(ex.__cxa_exception_type()->name()));
	}
}


bool Configurations::ReadCodecsFile()
{
	try
	{
		std::ifstream inFile;
		inFile.open("/tmp/Configs/Codecs.csv");
		if (!inFile) {
			_logger->error("Configurations::ReadCodecsFile", " Unable to open file: Codecs.csv");
			return false;
		}

		while (inFile.good())
		{
			std::string key, value;
			try
			{
				std::getline(inFile, key, ',');
				std::getline(inFile, value, '\n');
				trim(value);
				_logger->info("Configurations::ReadCodecsFile", "ReadCodecsFile - Key:" + key + "|Value:" + value);
				if (!key.empty() && !value.empty())
					_CodecTypesMap.insert({ key, std::stoi(value) });
				else
				{
					_logger->error("Configurations::ReadCodecsFile", "invalid configuration");
					return false;
				}
			}
			catch (...)
			{
				std::exception_ptr ex = std::current_exception();
				_logger->error("Configurations::ReadCodecsFile", " - Exception: " + string(ex.__cxa_exception_type()->name()));
				return false;
			}
		}
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Configurations::ReadCodecsFile", " Exception: " + string(ex.__cxa_exception_type()->name()));
		return false;
	}
	
}

bool Configurations::ReadPromptsFile()
{
	try
	{
		std::ifstream inFile;
		inFile.open("/tmp/Configs/Prompts.csv");
		//inFile.open("Prompts.csv");
		if (!inFile) {
			_logger->error("Configurations::ReadPromptsFile", " Unable to open file: Prompts.csv");
			return false;
		}

		while (inFile.good())
		{
			std::string key, value;
			try
			{
				std::getline(inFile, key, ',');
				std::getline(inFile, value, '\n');
				trim(key);
				trim(value);
//				_logger->info("Configurations::ReadPromptsFile", "ReadPromptsFile - Key:" + key + "|Value:" + value);
				if (!key.empty() && !value.empty())
				{
					int id;
					try {
						id = std::stoi(key);
					}
					catch (...)
					{
						std::exception_ptr ex = std::current_exception();
						_logger->error("Configurations::ReadPromptsFile", " Invalid Key " + key);
						continue;
					}
					
					_PromptsMap.insert({ id, value });
				}
				else
				{
					_logger->error("Configurations::ReadPromptsFile", "invalid configuration");
					return false;
				}
			}
			catch (...)
			{
				std::exception_ptr ex = std::current_exception();
				_logger->error("Configurations::ReadPromptsFile", " - Exception: " + string(ex.__cxa_exception_type()->name()));
				return false;
			}
		}
		return true;
	}
	catch (...)
	{
		std::exception_ptr ex = std::current_exception();
		_logger->error("Configurations::ReadPromptsFile", " Exception: " + string(ex.__cxa_exception_type()->name()));
		return false;
	}	
}
Configurations::Configurations()
{
	_logger = GlobalLogger::StaticLogger;
}

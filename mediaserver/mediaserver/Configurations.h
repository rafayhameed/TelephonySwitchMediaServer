#pragma once
#include <map>
#include"Common.h"

class Configurations
{
private:
	Logger * _logger = NULL;
public:

	static std::map<std::string, std::string> _configurations;
	static std::map<std::string, int> _CodecTypesMap;
	static std::map<int, std::string> _PromptsMap;
	void ReadConfigurations();
	bool ReadCodecsFile();
	bool ReadPromptsFile();
	Configurations();
};


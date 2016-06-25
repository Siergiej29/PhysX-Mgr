#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <time.h>

class Logger
{
public:
	Logger(std::string filename);
	~Logger();
protected:
	std::string filename;
	std::ofstream file;
public:
	bool log(std::string text);
};


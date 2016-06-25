#include "Logger.h"
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>


Logger::Logger(std::string filename)
{
	this->filename = filename;
	this->file.open(this->filename);
}


Logger::~Logger()
{
	this->file.close();
}


bool Logger::log(std::string text)
{
	if (this->file.is_open()) {
		this->file << text << std::endl;
		return true;
	} else 
		return false;
}

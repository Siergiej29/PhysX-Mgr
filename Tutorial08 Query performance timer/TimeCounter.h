#pragma once
#include <Windows.h>
#include <time.h>

class TimeCounter
{
public:
	TimeCounter();
	~TimeCounter();
	double stop();
	void reset();
	double getResult();
	double getStart();
	double PCFreq = 0.0;
	__int64 CounterStart = 0;
	LARGE_INTEGER frequency;        // ticks per second
	LARGE_INTEGER t2;    
	LARGE_INTEGER t1; // ticks
	double elapsedTime;
private:
	clock_t start;
	clock_t result;
};


#include "TimeCounter.h"
#include <Windows.h>



TimeCounter::TimeCounter()
{
	QueryPerformanceFrequency(&frequency);
	this->reset();
}


TimeCounter::~TimeCounter()
{
}


double TimeCounter::stop()
{
	QueryPerformanceCounter(&t2);
	return(this->result);
}

void TimeCounter::reset()
{
	QueryPerformanceCounter(&t1);
	//return(this->result);

}


double TimeCounter::getResult() {
	return (this->result);
}

double TimeCounter::getStart()
{
	

    // compute and print the elapsed time in millisec
    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
	return (double)elapsedTime;
}
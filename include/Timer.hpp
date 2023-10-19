#ifndef TIMER_HPP
#define TIMER_HPP

#include <iostream>
#include <sys/time.h>

class Timer {

private:
	int 			_timeoutSeconds;
	struct timeval	_lastRead;
	bool			_running;

public:
    Timer(int _timeoutSeconds);
	void start();
	void stop();
	bool hasTimeOut();
    ~Timer();
};

#endif

#ifndef TIMER_HPP
#define TIMER_HPP

#include <iostream>
#include <time.h>

class Timer {

private:
	int 	_timeout_seconds;
	clock_t _last_read;
	bool	_running;

public:
    Timer(int _timeout_seconds);
	void start();
	bool hasTimeOut();
    ~Timer();
};

#endif

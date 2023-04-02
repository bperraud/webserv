#include "Timer.hpp"

Timer::Timer(int timeout_seconds) : _timeout_seconds(timeout_seconds), _last_read(0), _running(false){

}

Timer::~Timer() {

}

void Timer::start() {
	_running = true;
	_last_read = clock();
}

bool Timer::hasTimeOut() {
	if (_running) {
		if ((clock() - _last_read) / CLOCKS_PER_SEC > _timeout_seconds) {
			_running = false;
			return true;
		}
	}
	return false;
}

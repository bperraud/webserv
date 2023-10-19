#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Utils.hpp"
#include "ServerConfig.hpp"
#include "ErrorHandler.hpp"
#include "Timer.hpp"
#include "CGIExecutor.hpp"

struct server;

class Client {

private:
	Timer		_timer;




public:

	void	startTimer();
	void	stopTimer();
	bool	hasTimeOut();

	int		writeToBody(char *buffer, ssize_t nbytes);



};

#endif

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
#include "HttpHandler.hpp"

struct server;

class Client {

private:
	std::stringstream 	_readWriteStream;

	Timer				_timer;
	HttpHandler			*_httpHandler;


	bool				_readyToWrite;
	ssize_t				_leftToRead;
	char				_overlapBuffer[4];

public:

	Client(int timeoutSeconds, server_name_level3 *serv_map);
	~Client();

	bool	isBodyUnfinished() const ;
	bool	hasBodyExceeded() const;
	bool	isKeepAlive() const;
	bool	isReadyToWrite() const;
	void	createHttpResponse();

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;

	void	setReadyToWrite(bool ready);
	void	writeToStream(char *buffer, ssize_t nbytes) ;
	int		writeToBody(char *buffer, ssize_t nbytes);
	void	saveOverlap(char *buffer, ssize_t nbytes);
	void	resetRequestContext();
	void	startTimer();
	void	stopTimer();
	bool	hasTimeOut();
	void	parseRequest();
};

#endif

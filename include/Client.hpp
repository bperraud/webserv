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
	std::stringstream   _request_body_stream;


	Timer				_timer;
	HttpHandler			*_httpHandler;
	WebSocketHandler	*_webSocketHandler;

	bool				_isHttpRequest;
	ssize_t				_lenStream;
	bool				_readyToWrite;
	ssize_t				_leftToRead;
	char				_overlapBuffer[4];

private :
	void	writeToHeader(char *buffer, ssize_t nbytes);
	void	parseRequest();

public:

	Client(int timeoutSeconds, server_name_level3 *serv_map);
	~Client();

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;


	bool	hasBodyExceeded() const;
	bool	isKeepAlive() const;
	bool	isReadyToWrite() const;

	void	createHttpResponse();
	int		treatReceivedData(char *buffer, ssize_t nbytes);
	void	setReadyToWrite(bool ready);
	int		writeToBody(char *buffer, ssize_t nbytes);

	void	saveOverlap(char *buffer, ssize_t nbytes);
	void	resetRequestContext();
	void	startTimer();
	void	stopTimer();
	bool	hasTimeOut();

};

#endif

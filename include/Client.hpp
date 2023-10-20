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
	Timer				_timer;

	bool				_ready_to_write;
	ssize_t				_leftToRead;

	char				_overlapBuffer[4];
	HttpHandler			*_httpHandler;

	//bool				_keepAlive;
	//bool				_body_size_exceeded;
	//bool				_transfer_chunked;

	//server_config*		_server;
	//routes				_default_route;
	//routes*				_active_route;

public:

	Client(int timeout_seconds, server_name_level3 *serv_map);
	~Client();


	bool	isBodyUnfinished() const ;
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

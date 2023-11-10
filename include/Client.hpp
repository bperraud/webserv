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
#include "bitset"

#include "Utils.hpp"
#include "ServerConfig.hpp"
#include "ErrorHandler.hpp"
#include "Timer.hpp"
#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"

#define MASKING_KEY_LEN 4
#define INITIAL_PAYLOAD_LEN 2

#define PAYLOAD_LENGTH_16 126
#define PAYLOAD_LENGTH_64 127

struct server;

class Client {

private:
	std::stringstream 	_requestHeaderStream;
	std::stringstream   _requestBodyStream;

	Timer				_timer;
	HttpHandler			*_httpHandler;
	WebSocketHandler	*_webSocketHandler;

	char				_maskingKey[4];

	bool				_isHttpRequest;
	bool				_readyToWrite;
	ssize_t				_lenStream;
	uint64_t			_leftToRead;
	char				_overlapBuffer[4];

private :
	void	writeToHeader(char *buffer, ssize_t nbytes);
	int		writeToStream(char *buffer, ssize_t nbytes);

public:

	Client(int timeoutSeconds, server_name_level3 *serv_map);
	~Client();

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;
	bool	hasBodyExceeded() const;
	bool	isKeepAlive() const;
	bool	isReadyToWrite() const;

	void	determineRequestType(char * &buffer);

	void	createResponse();
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

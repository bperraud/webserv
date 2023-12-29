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
#include "ProtocolHandlerInterface.hpp"


struct server;

class Client {

private:
	std::stringstream 	_requestHeaderStream;
	std::stringstream   _requestBodyStream;

	server_name_level3 *_serv_map;

	Timer				_timer;

	ProtocolHandlerInterface	*_protocolHandler;

	bool				_isHttpRequest;
	bool				_readyToWrite;
	ssize_t				_lenStream;


	uint64_t			_leftToRead;
	char				_overlapBuffer[4];

private :
	void	WriteToHeader(char *buffer, const ssize_t &nbytes);
	int		WriteToStream(char *buffer, const ssize_t &nbytes);
	int		WriteToBody(char *buffer, const ssize_t &nbytes);
	void	DetermineRequestType(char *buffer);

public:

	Client(int timeoutSeconds, server_name_level3 *serv_map);
	~Client();

	std::string		GetResponseHeader() const;
	std::string		GetResponseBody() const;
	bool	HasBodyExceeded() const;
	bool	IsKeepAlive() const;
	bool	IsReadyToWrite() const;

	void	CreateResponse();
	int		TreatReceivedData(char *buffer, const ssize_t &nbytes);
	void	SetReadyToWrite(bool ready);
	void	SaveOverlap(char *buffer, const ssize_t &nbytes);
	void	ResetRequestContext();
	void	StartTimer();
	void	StopTimer();
	bool	HasTimeOut();
};

#endif

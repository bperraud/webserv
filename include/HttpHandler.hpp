#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

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

# define EOF_CHUNKED "\r\n0\r\n\r\n"
# define CRLF "\r\n\r\n"
# define ROOT_PATH "www"
# define OVERLAP 4

struct server;

typedef std::map<std::string, server>	server_name_level3;

struct HttpMessage {
    std::string method;
    std::string url;
    std::string version;
	std::string host;
    std::map<std::string, std::string> map_headers;
    size_t bodyLength;
};

struct HttpResponse {
    std::string version;
    std::string status_code;
    std::string statusPhrase;
    std::map<std::string, std::string> map_headers;
};


class HttpHandler {

private:
	Timer				_timer;
	std::stringstream	_readStream;
	std::stringstream   _request_body_stream;
	std::stringstream   _response_header_stream;
	std::stringstream   _response_body_stream;
	HttpMessage			_request;
	HttpResponse		_response;
	char				_overlapBuffer[4];
	ssize_t				_leftToRead;

	static const std::map<std::string, std::string> _MIME_TYPES;
	static const std::map<int, std::string> _SUCCESS_STATUS;

	server_name_level3*	_serverMap;
	server_config*		_server;

	bool				_keepAlive;
	bool				_body_size_exceeded;
	bool				_ready_to_write;
	bool				_transfer_chunked;

	routes				_default_route;
	routes*				_active_route;

	void	assignServerConfig();
	void	createStatusResponse(int code);
	void	uploadFile(const std::string& contentType, size_t pos_boundary);
	void 	redirection();
	void	unchunckMessage();
	bool	findHeader(const std::string &header, std::string &value) const;
	bool	invalidRequest() const;

	void	GET();
	void	DELETE();
	void	POST();
	void	setupRoute(const std::string &url);
	bool	isAllowedMethod(const std::string &method) const;
	void	constructStringResponse();
	void	generateDirectoryListing(const std::string& directory_path);

public:
	HttpHandler(int timeout_seconds, server_name_level3 *serv_map);
	~HttpHandler();

	bool	isBodyUnfinished() const ;
	bool	isKeepAlive() const;
	bool	isReadyToWrite() const;
	void	createHttpResponse();

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;
	std::string		getContentType(const std::string& path) const;

	void	setReadyToWrite(bool ready);
	void	writeToStream(char *buffer, ssize_t nbytes) ;
	int		writeToBody(char *buffer, ssize_t nbytes);

	void	saveOverlap(char *buffer, ssize_t nbytes);
	void	resetRequestContext();
	void	startTimer();
	void	stopTimer();
	bool	hasTimeOut();

	void	handleCGI(const std::string &original_url);
	void	error(int error);

	void	parseRequest();

};

#endif

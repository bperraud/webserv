#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "Utils.hpp"
#include "ServerConfig.hpp"
#include "ErrorHandler.hpp"
#include "ServerError.hpp"
#include "ClientError.hpp"
#include "Timer.hpp"

# define CRLF "\r\n\r\n"
# define ROOT_PATH "./website"
# define DEFAULT_PAGE "./website/index.html"
# define UPLOAD_PATH "./website/upload/"


#if 0
Recipients of an invalid request-line SHOULD respond with either a
   400 (Bad Request) error or a 301 (Moved Permanently) redirect with
   the request-target properly encoded
#endif

struct HttpMessage {
    std::string method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> map_headers;
    bool has_body;
    size_t body_length;
};

struct HttpResponse {
    std::string version;
    std::string status_code;
    std::string status_phrase;
    std::map<std::string, std::string> map_headers;
};

class HttpHandler {

private:

	Timer				_timer;

	std::stringstream	*_readStream;
	std::stringstream   _request_body_stream;
	std::stringstream   _response_header_stream;
	std::stringstream   _response_body_stream;

	HttpMessage			_request;
	HttpResponse		_response;
	bool				_close_keep_alive;
	char				_last_4_char[4];

	ssize_t				_left_to_read;
	std::map<std::string, std::string> _MIME_TYPES;

	bool				_cgiMode;
	bool				_ready_to_write;

	server_config		_server;

	bool				_body_size_exceeded;

	routes*				_active_root;

	HttpHandler &operator=(HttpHandler const &other) {
		if (this != &other) {
			;
		}
		return *this;
	}


public:
	HttpHandler(int timeout_seconds, const server_config* serv);
	~HttpHandler();

	bool	isKeepAlive() const;

	void	writeToStream(char *buffer, ssize_t nbytes) ;
	int		writeToBody(char *buffer, ssize_t nbytes);

	void	startTimer() {
		_timer.start();
	}

	void	stopTimer() {
		_timer.stop();
	}

	bool	hasTimeOut() {
		return _timer.hasTimeOut();
	}

	bool	isReadyToWrite() const {
		return _ready_to_write;
	}

	void	setReadyToWrite(bool ready) {
		_ready_to_write = ready;
	}

	void error(int error) ;

	bool	isCGIMode() const {
		return _cgiMode;
	}

	HttpMessage getStructRequest() const {
		return _request;
	}

	std::string		getRequest() const  {
		return _readStream->str();
	}

	std::string		getBody() const {
		return _request_body_stream.str();
	}

	void	resetStream();

	void	copyLast4Char(char *buffer, ssize_t nbytes);

	ssize_t getLeftToRead() const {
		return _left_to_read;
	}

	std::string getResponseHeader() const {
		return _response_header_stream.str();
	}

	std::string getResponseBody() const {
		return _response_body_stream.str();
	}

	void parseRequest();
	void createHttpResponse();

	bool findHeader(const std::string &header, std::string &value);

	void GET();
	void POST();
	void DELETE();

	bool correctPath(const std::string& path) const;

	bool isCGI(const std::string &path) const ;
	void constructStringResponse();

	void findRoute(const std::string &url);

	void uploadFile(const std::string& contentType, size_t pos_boundary);

	std::string getContentType(const std::string& path) const;
};

#endif

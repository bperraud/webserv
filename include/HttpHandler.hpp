#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP


#include "ResponseServer.hpp"
#include "RequestClient.hpp"

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

enum Type {
	GET,
	POST,
	DELETE,
	HEAD
};

struct HttpMessage {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    bool has_body;
    size_t body_length;
    std::stringstream body_stream;
};

struct HttpResponse {
    std::string version;
    int status_code;
    std::string status_phrase;
    std::map<std::string, std::string> headers;
    std::stringstream body;
};

class HttpHandler {

private:

	std::stringstream	*_readStream;
	HttpMessage			_client;
	HttpResponse		_server;
	bool				_close_connection_mode;
	int					_type;
	char				_lastFour[4];

	ssize_t				_left_to_read;


	HttpHandler(const HttpHandler &copy) {
		*this = copy;
	}

	HttpHandler &operator=(HttpHandler const &other) {
		if (this != &other) {
			;
		}
		return *this;
	}


public:
	HttpHandler();
	~HttpHandler();

	//std::string addToRequest(const std::string &str);
	//std::string addToResponse(const std::string &str);

	//bool		hasBeenSend() const;
	bool		getConnectionMode() const;

	void	writeToStream(char *buffer, ssize_t nbytes) ;
	void	writeToBody(char *buffer, ssize_t nbytes);

	std::string		getRequest() {
		return _readStream->str();
	}

	std::string		getBody() {
		return _client.body_stream.str();
	}

	void	rmemcpy(char *buffer, ssize_t nbytes) {
		if (_lastFour[0])
			std::memcpy(buffer, _lastFour, 4);
		else
			std::memcpy(buffer, buffer + 4, 4);
		std::memcpy(_lastFour, buffer + nbytes, 4);		// copies last 4 bytes
	}

	ssize_t getLeftToRead() const {
		return _left_to_read;
	}

	ssize_t subLeftToRead(ssize_t i) {
		_left_to_read -= i;
		return _left_to_read;
	}

	int parseRequest(const std::string &http_message);

	void addFileToResponse(const std::string &fileName);
};

#endif


// no body ? end with \r\n\r\n
// GET and HEAD : no body
// POST and PUT), the presence of the Content-Length header or
// the Transfer-Encoding header indicates the length of the request body

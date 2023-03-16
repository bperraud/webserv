#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP


#include "ResponseServer.hpp"
#include "RequestClient.hpp"


#include <string>
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
    std::string http_version;
    int status_code;
    std::string reason_phrase;
    std::map<std::string, std::string> headers;
    std::stringstream body;
};

class HttpHandler {

private:

	std::stringstream		*_readStream;
	HttpMessage				_client;
	HttpResponse			_server;
	bool					_close_connection_mode;
	int						_type;

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

	void	writeToStream(char *buffer, ssize_t nbytes) {
		_readStream->write(buffer, nbytes);
	}

	std::string		getRequest() {
		return _readStream->str();
	}

	void addFileToResponse(const std::string &fileName);
};

#endif


// no body ? end with \r\n\r\n
// GET and HEAD : no body
// POST and PUT), the presence of the Content-Length header or
// the Transfer-Encoding header indicates the length of the request body

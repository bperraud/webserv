#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP


#include "ResponseServer.hpp"
#include "RequestClient.hpp"


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

enum Type {
	GET,
	POST,
	DELETE,
	HEAD
};

class HttpHandler {

private:

	std::stringstream		*_readStream;
	RequestClient			_client;
	ResponseServer			_server;
	bool					_close_connection_mode;
	int						_type;

public:
	HttpHandler();
	~HttpHandler();


	HttpHandler(const HttpHandler &copy) {
		_readStream = new std::stringstream ();
		*this = copy;
	}

	HttpHandler &operator=(HttpHandler const &other) {
		if (this != &other) {
			delete _readStream;
			_readStream = new std::stringstream(other._readStream->str());
			_client = other._client;
			_server = other._server;
			_close_connection_mode = other._close_connection_mode;
			_type = other._type;
		}
		return *this;
	}


	//std::string addToRequest(const std::string &str);
	//std::string addToResponse(const std::string &str);

	//bool		hasBeenSend() const;
	bool		getConnectionMode() const;
	std::string	getRequest() const;
	std::string	getResponse() const;

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

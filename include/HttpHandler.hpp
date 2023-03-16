#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

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
	std::string	_requestClient;
	std::string	_responseServer;
	bool		_close_connection_mode;
	int			_type;

public:
	HttpHandler();
	~HttpHandler();

	std::string addToRequest(const std::string &str);
	std::string addToResponse(const std::string &str);

	//bool		hasBeenSend() const;
	bool		getConnectionMode() const;
	std::string	getRequest() const;
	std::string	getResponse() const;

	void addFileToResponse(const std::string &fileName);
};

#endif


// no body ? end with \r\n\r\n
// GET and HEAD : no body
// POST and PUT), the presence of the Content-Length header or
// the Transfer-Encoding header indicates the length of the request body

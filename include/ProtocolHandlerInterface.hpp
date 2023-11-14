#ifndef PROTOCOLHANDLERINTERFACE_HPP
#define PROTOCOLHANDLERINTERFACE_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class ProtocolHandlerInterface {

protected:
	std::stringstream   _response_header_stream;
	std::stringstream   _response_body_stream;

public:
    ProtocolHandlerInterface() = default;  // default constructor

	virtual ~ProtocolHandlerInterface() = default;

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;

	virtual size_t	getPositionEndHeader(char *buffer) = 0;
	virtual int 	writeToBody(std::stringstream &bodyStream, char* buffer,
						const ssize_t &nbytes, u_int64_t &leftToRead) = 0;
	virtual bool	hasBodyExceeded() const = 0;
	virtual bool	isKeepAlive() const = 0;
	virtual void	createHttpResponse(std::stringstream &bodyStream) = 0;
	virtual void	resetRequestContext() = 0;
	virtual int		parseRequest(std::stringstream &headerStream) = 0;
};

#endif

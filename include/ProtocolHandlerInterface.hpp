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
    ProtocolHandlerInterface() = default;  // Deleting the constructor

	virtual ~ProtocolHandlerInterface() = default;

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;

	virtual bool	hasBodyExceeded() const = 0;
	virtual bool 	isBodyFinished(std::stringstream &bodyStream, uint64_t &leftToRead, ssize_t nbytes) = 0;
	virtual bool	isKeepAlive() const = 0;

	virtual bool 	bodyExceeded(std::stringstream &bodyStream, ssize_t nbytes) = 0;
	virtual void	createHttpResponse(std::stringstream &bodyStream) = 0;


	virtual void	resetRequestContext() = 0;
	virtual int		parseRequest(std::stringstream &_readStream) = 0;

};

#endif

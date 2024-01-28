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

	std::string		GetResponseHeader() const;
	std::string		GetResponseBody() const;

	virtual size_t	GetPositionEndHeader(char *buffer) = 0;
	virtual int 	WriteToBody(std::stringstream &bodyStream, char* buffer,
						const ssize_t &nbytes) = 0;
	virtual bool	HasBodyExceeded() const = 0;
	virtual bool	IsKeepAlive() const = 0;
	//virtual void	CreateHttpResponse(std::stringstream &bodyStream) = 0;

    virtual void	CreateHttpResponse(char * buffer, uint64_t size) = 0;

	virtual void	ResetRequestContext() = 0;
	virtual int		ParseRequest(std::stringstream &headerStream) = 0;
};

#endif

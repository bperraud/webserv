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
    ProtocolHandlerInterface() = default;

	virtual ~ProtocolHandlerInterface() = default;

	std::string		GetResponseHeader() const;
	std::string		GetResponseBody() const;

	virtual size_t	GetPositionEndHeader(char *buffer) = 0;
	virtual int 	WriteToBody(char * request_body, const uint64_t &hasBeenRead, char* buffer, const ssize_t &nbytes) = 0;
	virtual bool	HasBodyExceeded() const = 0;
	virtual bool	IsKeepAlive() const = 0;
    virtual void	CreateHttpResponse(char * buffer, const uint64_t &size) = 0;

	virtual void	ResetRequestContext() = 0;
	virtual int		ParseRequest(std::stringstream &headerStream) = 0;
};

#endif

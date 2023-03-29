#ifndef SERVERERROR_HPP
#define SERVERERROR_HPP

#include "ErrorHandler.hpp"

class ServerError : public ErrorHandler {

private:


public:
    ServerError(HttpMessage& request, std::stringstream& stream);

	void errorProcess(int error);

	void internalServerError();
	void notImplemented();
	void gatewayTimeout();
	void HTTPVersion();

	~ServerError();
};

#endif

#ifndef SERVERERROR_HPP
#define SERVERERROR_HPP

#include "ErrorHandler.hpp"

#include <map>

class ServerError : public ErrorHandler {

private:
	std::map<int, void(ServerError::*)()> _error_map;

public:
    ServerError(HttpResponse& request, std::stringstream& body_stream);
	~ServerError();
	void errorProcess(int error);
	void internalServerError();
	void notImplemented();
	void gatewayTimeout();
	void HTTPVersion();
};

#endif

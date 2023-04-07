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
	void internalServerError(); // 500
	void notImplemented(); 		// 501
	void gatewayTimeout(); 		// 504
	void HTTPVersion(); 		// 505
};

#endif

#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include <sstream>
#include "HttpHandler.hpp"

class ErrorHandler {

private:
    HttpMessage&		_request;
    std::stringstream&	_stream;

protected:
	ErrorHandler(HttpMessage& request, std::stringstream& stream);

public:
	ErrorHandler();
	virtual void errorProcess(int error) = 0;
};

#endif

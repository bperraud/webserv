#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include <sstream>

struct HttpResponse;

class ErrorHandler {

protected:
    HttpResponse&		_response;
    std::stringstream&	_body_stream;

public:
	ErrorHandler(HttpResponse& response, std::stringstream& body_stream);
	ErrorHandler();
	virtual void errorProcess(int error) = 0;
	virtual ~ErrorHandler() {};
};

#endif

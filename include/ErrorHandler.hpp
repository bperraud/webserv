#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include <sstream>
#include <map>

struct HttpResponse;

class ErrorHandler {

protected:
	static const std::map<int, std::string> ERROR_MAP;

	HttpResponse&		_response;
	std::stringstream&	_body_stream;
	std::string			_error_page;
public:
	ErrorHandler(HttpResponse& response, std::stringstream& body_stream, const std::string &error_page);
	void errorProcess(int error);
	virtual ~ErrorHandler() {};
};

class ClientError : public ErrorHandler {

public:
	ClientError(HttpResponse& response, std::stringstream& body_stream, const std::string &error_page);
	~ClientError();
};

class ServerError : public ErrorHandler {

public:
	ServerError(HttpResponse& request, std::stringstream& body_stream, const std::string &error_page);
	~ServerError();
};

#endif

#ifndef RESPONSESERVER_HPP
#define RESPONSESERVER_HPP

#include <string>


class ResponseServer {

private:
	std::string	_response;

public:
    ResponseServer();
	std::string	getResponse() const;
    ~ResponseServer();
};

#endif

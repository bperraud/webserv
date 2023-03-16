#ifndef REQUESTCLIENT_HPP
#define REQUESTCLIENT_HPP

#include <string>

class RequestClient {

private:
	std::string	_request;

public:
    RequestClient();

	std::string addToRequest(const std::string &str) ;

	std::string	getRequest() const;

    ~RequestClient();
};

#endif

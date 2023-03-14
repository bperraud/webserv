#ifndef CLIENTREQUEST_HPP
#define CLIENTREQUEST_HPP

#include <string>

enum Type {
	GET,
	POST,
	DELETE,
	HEAD
};

class ClientRequest {

private:
	std::string	_request;
	bool		_close_connection_mode;
	bool		_sended;

public:
	ClientRequest();
	~ClientRequest();

	void addToRequest(const std::string &str);

	bool		hasBeenSend() const;
	bool		getConnectionMode() const;
	std::string	getRequest() const;
};

#endif


// no body ? end with \r\n\r\n
// GET and HEAD : no body
// POST and PUT), the presence of the Content-Length header or
// the Transfer-Encoding header indicates the length of the request body

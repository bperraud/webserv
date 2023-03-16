#include "ResponseServer.hpp"


ResponseServer::ResponseServer() {
	_response = "HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html\r\n"
					"Content-Length: 30\r\n"
					"Connection: close\r\n\r\n"
					"<html><body>Hello my world!</body></html>";
}

std::string ResponseServer::getResponse() const {
	return _response;
}

ResponseServer::~ResponseServer() {

}

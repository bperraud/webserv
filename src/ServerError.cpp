#include "ServerError.hpp"
#include "HttpHandler.hpp"

ServerError::ServerError(HttpResponse& response, std::stringstream& body_stream) : ErrorHandler(response, body_stream), _error_map() {
	_error_map[500] = &ServerError::internalServerError;
	_error_map[501] = &ServerError::notImplemented;
	_error_map[504] = &ServerError::gatewayTimeout;
	_error_map[505] = &ServerError::HTTPVersion;
}

ServerError::~ServerError() {

}

void ServerError::errorProcess(int error) {
	if (_error_map.find(error) == _error_map.end())
		return ;
	(this->*_error_map[error])();
	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = Utils::intToString(_body_stream.str().length());
}

void ServerError::internalServerError() {
	_response.status_code = "500";
	_response.status_phrase = "Internal Server Error";
	_body_stream << "<html><body><h1>500 Internal Server Error</h1></body></html>";
}

void ServerError::notImplemented() {
	_response.status_code = "501";
	_response.status_phrase = "Not Implemented";
	_body_stream << "<html><body><h1>501 Not Implemented</h1></body></html>";
}

void ServerError::gatewayTimeout() {
	_response.status_code = "504";
	_response.status_phrase = "Gateway Timeout";
	_body_stream << "<html><body><h1>504 Gateway Timeout</h1></body></html>";
}

void ServerError::HTTPVersion() {
	_response.status_code = "505";
	_response.status_phrase = "HTTP Version Not Supported";
	_body_stream << "<html><body><h1>505 HTTP Version Not Supported</h1></body></html>";
}

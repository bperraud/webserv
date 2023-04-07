#include "ClientError.hpp"
#include "HttpHandler.hpp"

ClientError::ClientError(HttpResponse& response, std::stringstream& stream) : ErrorHandler(response, stream) {
	_error_map[400] = &ClientError::badRequest;
	_error_map[403] = &ClientError::forbidden;
	_error_map[404] = &ClientError::notFound;
	_error_map[405] = &ClientError::methodNotAllowed;
	_error_map[408] = &ClientError::timeout;
	_error_map[413] = &ClientError::payloadTooLarge;
	_error_map[415] = &ClientError::unsupportedMediaType;
}

ClientError::~ClientError() {

}

void ClientError::errorProcess(int error) {
	if (_error_map.find(error) == _error_map.end())
		return ;
	(this->*_error_map[error])();
	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = Utils::intToString(_body_stream.str().length());
}

void ClientError::badRequest() {
	_response.status_code = "400";
	_response.status_phrase = "Bad Request";
	_body_stream << "<html><body><h1>400 Bad Request</h1></body></html>";
}

void ClientError::forbidden() {
	_response.status_code = "403";
	_response.status_phrase = "Forbidden";
	_body_stream << "<html><body><h1>403 Forbidden</h1></body></html>";
}

void ClientError::notFound() {
	_response.status_code = "404";
	_response.status_phrase = "Not Found";
	_body_stream << "<html><body><h1>404 Not Found</h1></body></html>";
}

void ClientError::methodNotAllowed() {
	_response.status_code = "405";
	_response.status_phrase = "Method Not Allowed";
	_body_stream << "<html><body><h1>405 Method Not Allowed</h1></body></html>";
}

void ClientError::timeout() {
	_response.status_code = "408";
	_response.status_phrase = "Request Timeout";
	_body_stream << "<html><body><h1>408 Request Timeout</h1></body></html>";
}

void ClientError::payloadTooLarge() {
	_response.status_code = "413";
	_response.status_phrase = "Payload Too Large";
	_body_stream << "<html><body><h1>413 Payload Too Large</h1></body></html>";
}

void ClientError::unsupportedMediaType() {
	_response.status_code = "415";
	_response.status_phrase = "Unsupported Media Type";
	_body_stream << "<html><body><h1>415 Unsupported Media Type</h1></body></html>";
}

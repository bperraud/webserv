#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _requestClient(), _close_connection_mode(false), _type(0){

}

HttpHandler::~HttpHandler() {

}

std::string HttpHandler::addToRequest(const std::string &str) {
	_requestClient += str;
	return _requestClient;
}

std::string HttpHandler::addToResponse(const std::string &str) {
	_responseServer += str;
	return _responseServer;
}

//bool HttpHandler::hasBeenSend() const {
//	return _sended;
//}

bool HttpHandler::getConnectionMode() const {
	return _close_connection_mode;
}

std::string HttpHandler::getRequest() const {
	return _requestClient;
}

void HttpHandler::addFileToResponse(const std::string &fileName) {
	std::ifstream input_file(fileName.c_str());
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    _responseServer += buffer.str();
}

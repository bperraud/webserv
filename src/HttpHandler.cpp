#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _readStream(new std::stringstream()),  _close_connection_mode(false), _type(0){

}

HttpHandler::~HttpHandler() {
	delete _readStream;
}

//std::string HttpHandler::addToRequest(const std::string &str) {
//	_client._request += str;
//	return _client._request;
//}

//std::string HttpHandler::addToResponse(const std::string &str) {
//	_server._responseServer += str;
//	return _server._response;
//}

//bool HttpHandler::hasBeenSend() const {
//	return _sended;
//}

bool HttpHandler::getConnectionMode() const {
	return _close_connection_mode;
}

std::string HttpHandler::getRequest() const {
	return _client.getRequest();
}

std::string HttpHandler::getResponse() const {
	return _server.getResponse();
}

void HttpHandler::addFileToResponse(const std::string &fileName) {
	std::ifstream input_file(fileName.c_str());
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    //_server._response += buffer.str();
}

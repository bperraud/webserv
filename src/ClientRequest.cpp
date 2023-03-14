#include "ClientRequest.hpp"


ClientRequest::ClientRequest() {

}

ClientRequest::~ClientRequest() {

}

std::string ClientRequest::addToRequest(const std::string &str) {
	_request += str;
	return _request;
}

bool ClientRequest::hasBeenSend() const {
	return _sended;
}

bool ClientRequest::getConnectionMode() const {
	return _close_connection_mode;
}

std::string ClientRequest::getRequest() const {
	return _request;
}

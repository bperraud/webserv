#include "Client.hpp"

// --------------------------------- GETTERS --------------------------------- //

std::string Client::getResponseHeader() const { return _httpHandler->getResponseHeader(); }
std::string Client::getResponseBody() const { return _httpHandler->getResponseBody(); }

bool Client::isKeepAlive() const { return _httpHandler->isKeepAlive(); }

bool Client::isReadyToWrite() const { return _ready_to_write; }

Client::Client(int timeout_seconds, server_name_level3 *serv_map) :_timer(timeout_seconds), _ready_to_write(false), _httpHandler(nullptr),
	_overlapBuffer() {
	_httpHandler = new HttpHandler (timeout_seconds, serv_map);
	_overlapBuffer[0] = '\0';
}

bool Client::isBodyUnfinished() const {
	return _httpHandler->isBodyUnfinished();
}

// --------------------------------- SETTERS --------------------------------- //

void Client::setReadyToWrite(bool ready) { _ready_to_write = ready; }

// ---------------------------------- TIMER ---------------------------------- //

void Client::startTimer() { _timer.start(); }

void Client::stopTimer() { _timer.stop(); }

bool Client::hasTimeOut() { return _timer.hasTimeOut(); }

void Client::resetRequestContext() {
	bzero(_overlapBuffer, OVERLAP);
	_httpHandler->resetRequestContext();
}

void Client::writeToStream(char *buffer, ssize_t nbytes) {
	_httpHandler->writeToStream(buffer, nbytes);
}

int Client::writeToBody(char *buffer, ssize_t nbytes) {
	return _httpHandler->writeToBody(buffer, nbytes);
}

void Client::saveOverlap(char *buffer, ssize_t nbytes)
{
	if (nbytes >= OVERLAP)
	{
		if (_overlapBuffer[0])
			std::memcpy(buffer, _overlapBuffer, OVERLAP);
		else
			std::memcpy(buffer, buffer + OVERLAP, OVERLAP);
		std::memcpy(_overlapBuffer, buffer + nbytes, OVERLAP); // save last 4 char
	}
	else if (nbytes)
	{
		std::memcpy(buffer, _overlapBuffer, OVERLAP);
		std::memmove(_overlapBuffer, _overlapBuffer + nbytes, OVERLAP - nbytes); // moves left by nbytes
		std::memcpy(_overlapBuffer + OVERLAP - nbytes, buffer + OVERLAP, nbytes);	// save last nbytes char
	}
}

void Client::createHttpResponse() {
	_httpHandler->createHttpResponse();
}

void Client::parseRequest() {
	_httpHandler->parseRequest();
}

Client::~Client() {
	delete _httpHandler;
}

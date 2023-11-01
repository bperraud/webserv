#include "Client.hpp"

// --------------------------------- GETTERS --------------------------------- //

std::string Client::getResponseHeader() const { return _httpHandler->getResponseHeader(); }
std::string Client::getResponseBody() const { return _httpHandler->getResponseBody(); }
bool Client::isKeepAlive() const { return _httpHandler->isKeepAlive(); }
bool Client::isReadyToWrite() const { return _readyToWrite; }

Client::Client(int timeoutSeconds, server_name_level3 *serv_map) : _readWriteStream(std::ios::in | std::ios::out), _timer(timeoutSeconds), _readyToWrite(false), _httpHandler(nullptr),
	_overlapBuffer() {
	_httpHandler = new HttpHandler(timeoutSeconds, serv_map);
	_overlapBuffer[0] = '\0';
}

bool Client::isBodyUnfinished() const {
	return _httpHandler->isBodyUnfinished();
}

bool Client::hasBodyExceeded() const {
	return _httpHandler->hasBodyExceeded();
}

// --------------------------------- SETTERS --------------------------------- //

void Client::setReadyToWrite(bool ready) { _readyToWrite = ready; }

// ---------------------------------- TIMER ---------------------------------- //

void Client::startTimer() { _timer.start(); }
void Client::stopTimer() { _timer.stop(); }
bool Client::hasTimeOut() { return _timer.hasTimeOut(); }

void Client::resetRequestContext() {
	bzero(_overlapBuffer, OVERLAP);
	_httpHandler->resetRequestContext();
	_readyToWrite = false;
	_readWriteStream.str(std::string());
	_readWriteStream.seekp(0, std::ios_base::beg);
	_readWriteStream.clear();
}

void Client::writeToStream(char *buffer, ssize_t nbytes) {
	_readWriteStream.write(buffer, nbytes);
	if (_readWriteStream.fail()) {
		std::ios::iostate state = _readWriteStream.rdstate();
		std::cout << state << std::endl;
		throw std::runtime_error("writing to read stream");
	}
}

int Client::writeToBody(char *buffer, ssize_t nbytes) {
	return _httpHandler->writeToBody(buffer, nbytes);
}

void Client::saveOverlap(char *buffer, ssize_t nbytes) {
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
		std::memcpy(_overlapBuffer + OVERLAP - nbytes, buffer + OVERLAP, nbytes); // save last nbytes char
	}
}

void Client::createHttpResponse() {
	_httpHandler->createHttpResponse();
}

void Client::parseRequest() {
	_httpHandler->parseRequest(_readWriteStream);
}

Client::~Client() {
	delete _httpHandler;
}

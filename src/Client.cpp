#include "Client.hpp"

// --------------------------------- GETTERS --------------------------------- //

std::string Client::getResponseHeader() const { return _httpHandler->getResponseHeader(); }
std::string Client::getResponseBody() const { return _httpHandler->getResponseBody(); }
bool Client::isKeepAlive() const { return _httpHandler->isKeepAlive(); }
bool Client::isReadyToWrite() const { return _readyToWrite; }

Client::Client(int timeoutSeconds, server_name_level3 *serv_map) : _requestHeaderStream(std::ios::in | std::ios::out),
	_requestBodyStream(std::ios::in | std::ios::out), _timer(timeoutSeconds), _httpHandler(nullptr),
	_webSocketHandler(nullptr), _isHttpRequest(true), _readyToWrite(false), _lenStream(0),
	_overlapBuffer(), _leftToRead(0) {
	_httpHandler = new HttpHandler(timeoutSeconds, serv_map);
	_webSocketHandler = new WebSocketHandler();
	_overlapBuffer[0] = '\0';
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
	_lenStream = 0;
	_leftToRead = 0;
	bzero(_overlapBuffer, OVERLAP);
	_httpHandler->resetRequestContext();
	_readyToWrite = false;
	_requestHeaderStream.str(std::string());
	_requestHeaderStream.seekp(0, std::ios_base::beg);
	_requestHeaderStream.clear();
	_requestBodyStream.str(std::string());
	_requestBodyStream.seekp(0, std::ios_base::beg);
	_requestBodyStream.clear();
}

void Client::determineRequestType(char *buffer) {
	std::bitset<8> firstByte(buffer[0]);
	std::bitset<8> secondByte(buffer[1]);
	std::cout << firstByte << " ";
	std::cout << secondByte << " " << std::endl;

	const bool mask_bit = secondByte[7];
	if (mask_bit == 1)	{ // bit Mask 1 = websocket
		secondByte[7] = 0;
		//_leftToRead =

		unsigned int payload = secondByte.to_ulong();

		if (payload == 126) {
			uint64_t t = buffer[1];
			_leftToRead = t;
		}
		else if (payload == 127)
			_leftToRead = buffer[1];
		else
			_leftToRead = payload;

		std::memcpy(_maskingKey, buffer + INITIAL_PAYLOAD_LEN, MASKING_KEY_LEN);
		std::cout << "payloadLength : " << _leftToRead << std::endl;
		_isHttpRequest = false;
	}
	else
		_isHttpRequest = true;
}

void Client::writeToHeader(char *buffer, ssize_t nbytes) {
	_requestHeaderStream.write(buffer, nbytes);
	_lenStream += nbytes;
	if (_requestHeaderStream.fail()) {
		std::ios::iostate state = _requestHeaderStream.rdstate();
		std::cout << state << std::endl;
		throw std::runtime_error("writing to read stream");
	}
}

int Client::writeToBody(char *buffer, ssize_t nbytes) {
	if (_httpHandler->bodyExceeded(_requestBodyStream, nbytes))
		return 0;
	_requestBodyStream.write(buffer, nbytes);
	if (_requestBodyStream.fail())
		throw std::runtime_error("writing to request body stream");
	if (_leftToRead) { // not chunked
		_leftToRead -= nbytes;
		return _leftToRead > 0;
	}
	return _httpHandler->transferChunked(_requestBodyStream); // chunked
}

int Client::writeToStream(char *buffer, ssize_t nbytes) {
	if (!_isHttpRequest) {
		buffer += 6;

		std::string res;
		for (int i = 0; i < _leftToRead; i++) {
			unsigned char unmaskedByte = buffer[i] ^ _maskingKey[i % 4];
			res += unmaskedByte;
		}
		std::cout << res << std::endl;
		writeToBody(buffer, _leftToRead);
		return (0);
	}

	bool isBodyUnfinished = (_leftToRead || _httpHandler->isTransferChunked());
	if (isBodyUnfinished) {
		isBodyUnfinished = writeToBody(buffer, nbytes);
		return (isBodyUnfinished);
	}
	const size_t pos_end_header = ((std::string)(buffer - 4)).find(CRLF);
	if (pos_end_header == std::string::npos) {
		writeToHeader(buffer, nbytes);
		return (1);
	}
	writeToHeader(buffer, pos_end_header);
	_leftToRead = _httpHandler->parseRequest(_requestHeaderStream);
	isBodyUnfinished = writeToBody(buffer + pos_end_header, nbytes - pos_end_header);
	return (isBodyUnfinished);
}

int Client::treatReceivedData(char *buffer, ssize_t nbytes) {
	startTimer();
	saveOverlap(buffer - OVERLAP, nbytes);
	if (_lenStream == 0 && nbytes >= 2) // socket frame or http ?
		determineRequestType(buffer);
	return (writeToStream(buffer, nbytes));
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

void Client::createResponse() {
	_httpHandler->createHttpResponse(_requestBodyStream);
}

Client::~Client() {
	delete _httpHandler;
}

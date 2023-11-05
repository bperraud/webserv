#include "Client.hpp"
#include "bitset"

// --------------------------------- GETTERS --------------------------------- //

std::string Client::getResponseHeader() const { return _httpHandler->getResponseHeader(); }
std::string Client::getResponseBody() const { return _httpHandler->getResponseBody(); }
bool Client::isKeepAlive() const { return _httpHandler->isKeepAlive(); }
bool Client::isReadyToWrite() const { return _readyToWrite; }

Client::Client(int timeoutSeconds, server_name_level3 *serv_map) : _readWriteStream(std::ios::in | std::ios::out), _request_body_stream(std::ios::in | std::ios::out),
	 _timer(timeoutSeconds), _readyToWrite(false), _httpHandler(nullptr),
	_isHttpRequest(true), _lenStream(0), _overlapBuffer(), _leftToRead(0) {
	_httpHandler = new HttpHandler(timeoutSeconds, serv_map);
	//_webSocketHandler = new WebSocketHandler();
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
	_readWriteStream.str(std::string());
	_readWriteStream.seekp(0, std::ios_base::beg);
	_readWriteStream.clear();
	_request_body_stream.str(std::string());
	_request_body_stream.seekp(0, std::ios_base::beg);
	_request_body_stream.clear();
}

void Client::writeToHeader(char *buffer, ssize_t nbytes) {
	_readWriteStream.write(buffer, nbytes);

	std::cout << nbytes << std::endl;

	if (_lenStream == 0 && nbytes >= 2) { // socket frame or http ?

		std::bitset<8> firstByte(buffer[0]);
		std::bitset<8> secondByte(buffer[1]);
		std::cout << firstByte << " ";
		std::cout << secondByte << " " << std::endl;

		bool mask_bit = secondByte[7];
		secondByte[7] = 0;
		unsigned int payloadLength = secondByte.to_ulong();
		std::cout << "payloadLength : " << payloadLength << std::endl;
		if (mask_bit == 1)	{ // bit Mask 1 = websocket
			std::cout << "websocket" << std::endl;
			_isHttpRequest = false;
		}
	}
	_lenStream += nbytes;
	if (_readWriteStream.fail()) {
		std::ios::iostate state = _readWriteStream.rdstate();
		std::cout << state << std::endl;
		//throw std::runtime_error("writing to read stream");
	}
}

int Client::treatReceivedData(char *buffer, ssize_t nbytes) {
	startTimer();
	saveOverlap(buffer, nbytes);
	bool isBodyUnfinished = (_leftToRead || _httpHandler->isBodyUnfinished());
	if (isBodyUnfinished) {
		isBodyUnfinished = writeToBody(buffer + OVERLAP, nbytes);
		return (isBodyUnfinished);
	}
	const size_t pos_end_header = ((std::string)buffer).find(CRLF);
	if (pos_end_header == std::string::npos) {
		writeToHeader(buffer + OVERLAP, nbytes);
		return (1);
	} else {
		writeToHeader(buffer + OVERLAP, pos_end_header);
		_leftToRead = _httpHandler->parseRequest(_readWriteStream);
		isBodyUnfinished = writeToBody(buffer + OVERLAP + pos_end_header, nbytes - pos_end_header);
		return (isBodyUnfinished);
	}
}

int Client::writeToBody(char *buffer, ssize_t nbytes) {
	//return

	if (_httpHandler->bodyExceeded(_request_body_stream, nbytes))
		return 0;

	_request_body_stream.write(buffer, nbytes);
	if (_request_body_stream.fail())
		throw std::runtime_error("writing to request body stream");

	if (_leftToRead) { // not chunked
		_leftToRead -= nbytes;
		return _leftToRead > 0;
	}
	// chunked
	return _httpHandler->writeToBody(_request_body_stream, nbytes);
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
	_httpHandler->createHttpResponse(_request_body_stream);
}

//void Client::parseRequest() {
//	_httpHandler->parseRequest(_readWriteStream);
//}

Client::~Client() {
	delete _httpHandler;
}

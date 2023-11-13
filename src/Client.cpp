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

void Client::determineRequestType(char * &header) {
	const bool mask_bit = *(header + 1) >> 7;
	if (mask_bit == 1)	{ // bit Mask 1 = websocket
		//u_int8_t payload = header[1];
		//payload &= 0x7f;
		//size_t payloadLenBytes = 0;
		//_leftToRead = payload;
		//if (payload == PAYLOAD_LENGTH_16)
		//	payloadLenBytes = 2;
		//else if (payload == PAYLOAD_LENGTH_64)
		//	payloadLenBytes = 8;

		//if (payloadLenBytes) {
		//	memcpy(&_leftToRead, header + 2, payloadLenBytes);
		//	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		//	_leftToRead = ntohs(_leftToRead);
		//	#endif
		//}

		//_webSocketHandler = new WebSocketHandler(header);
		//header += INITIAL_PAYLOAD_LEN + MASKING_KEY_LEN + payloadLenBytes;
		//std::cout << "payloadLength : " << _leftToRead << std::endl;
		//std::memcpy(_maskingKey, header - MASKING_KEY_LEN, MASKING_KEY_LEN);

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

	return _httpHandler->writeToBody(_requestBodyStream, buffer, nbytes, _leftToRead);
}


// question to chat gpt : why the need for host in http request ? (body size websocket)
int Client::writeToStream(char *buffer, ssize_t nbytes) {
	//if (!_isHttpRequest) {	// pb : cannot call base class function
	//	std::string res;
	//	for (int i = 0; i < _leftToRead; i++) {
	//		unsigned char unmaskedByte = buffer[i] ^ _maskingKey[i % 4];
	//		res += unmaskedByte;
	//	}
	//	std::cout << res << std::endl;
	//	writeToBody(buffer, _leftToRead);
	//	//writeToBody(buffer, nbytes); ?
	//	return (0);
	//}

	if (_leftToRead)
		return writeToBody(buffer, nbytes);

	const size_t pos_end_header = _httpHandler->getPositionEndHeader(buffer);

	//const size_t pos_end_header = ((std::string)(buffer - 4)).find(CRLF);
	if (pos_end_header == std::string::npos) {
		writeToHeader(buffer, nbytes);
		return (1);
	}
	writeToHeader(buffer, pos_end_header);
	_leftToRead = _httpHandler->parseRequest(_requestHeaderStream);
	return writeToBody(buffer + pos_end_header, nbytes - pos_end_header);
}

int Client::treatReceivedData(char *buffer, ssize_t nbytes) {
	startTimer();
	saveOverlap(buffer - OVERLAP, nbytes);
	if (_lenStream == 0 && nbytes >= 2) { // socket frame or http ?  -> remove the nbytes 2
		determineRequestType(buffer);
	}
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

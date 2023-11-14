#include "WebSocketHandler.hpp"
#include "HttpHandler.hpp"

const std::map<int, std::string> WebSocketHandler::_OPCODE_MAP = {
	{OPCODE_CONT, "cont"},
    {OPCODE_TEXT, "text"},
    {OPCODE_BINARY, "binary"},
    {OPCODE_CLOSE, "close"},
    {OPCODE_PING, "ping"},
    {OPCODE_PONG, "pong"}
};

WebSocketHandler::WebSocketHandler() {

}

WebSocketHandler::WebSocketHandler(char *header) {
	std::bitset<8> fb(header[0]);
	std::cout << fb << std::endl;

	_opcode = 0;
	_fin = header[0] >> 7 & 1;
    _rsv1 = header[0] >> 6 & 1;
    _rsv2 = header[0] >> 5 & 1;
    _rsv3 = header[0] >> 4 & 1;
	_opcode = header[0] & 0xf;

	std::cout << _fin << " ";
	std::cout << _rsv1 << " ";
	std::cout << _rsv2 << " ";
	std::cout << _rsv3 << " ";
	std::cout << "_opcode : " << +_opcode << " " << std::endl;

}

bool WebSocketHandler::isKeepAlive() const { return true; }

void WebSocketHandler::resetRequestContext() {
	_response_body_stream.str(std::string());
	_response_body_stream.clear();
	_response_header_stream.str(std::string());
	_response_header_stream.clear();
}

size_t WebSocketHandler::getPositionEndHeader(char *header) {
	// if < 2 : return std:string::npos

	u_int8_t payload = header[1];
	payload &= 0x7f;
	size_t payloadLenBytes = 0;
	_leftToRead = payload;
	if (payload == PAYLOAD_LENGTH_16)
		payloadLenBytes = 2;
	else if (payload == PAYLOAD_LENGTH_64)
		payloadLenBytes = 8;

	if (payloadLenBytes) {
		memcpy(&_leftToRead, header + 2, payloadLenBytes);
		#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		_leftToRead = ntohs(_leftToRead);
		#endif
	}

	size_t pos_end_header = INITIAL_PAYLOAD_LEN + MASKING_KEY_LEN + payloadLenBytes;
	std::cout << "payloadLength : " << _leftToRead << std::endl;

	std::memcpy(_maskingKey, header + INITIAL_PAYLOAD_LEN + payloadLenBytes, MASKING_KEY_LEN);

	return pos_end_header;
}

int WebSocketHandler::writeToBody(std::stringstream &bodyStream, char *buffer, const ssize_t &nbytes, u_int64_t &leftToRead) {
	std::string res;
	for (int i = 0; i < _leftToRead; i++) {
		const char unmaskedByte = buffer[i] ^ _maskingKey[i % 4];
		res += unmaskedByte;
		bodyStream.write(&unmaskedByte, sizeof(unmaskedByte));
	}
	//std::cout << res << std::endl;

	std::cout << bodyStream.str() << std::endl;
	//bodyStream.write(buffer, nbytes);
	//if (bodyStream.fail())
	//	throw std::runtime_error("writing to request body stream");

	leftToRead -= nbytes;
	return leftToRead > 0;
}

bool WebSocketHandler::hasBodyExceeded() const {
	return false;
}

void WebSocketHandler::createHttpResponse(std::stringstream &bodyStream) {

}

int WebSocketHandler::parseRequest(std::stringstream &headerStream) {
	return _leftToRead;
}

void WebSocketHandler::writeHeaderStream() {

}

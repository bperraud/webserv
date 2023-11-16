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
	//std::bitset<8> fb(header[0]);
	//std::cout << fb << std::endl;

	_byte = 0;
	_opcode = 0;
	_fin = header[0] >> 7 & 1;
    _rsv1 = header[0] >> 6 & 1;
    _rsv2 = header[0] >> 5 & 1;
    _rsv3 = header[0] >> 4 & 1;
	_opcode = header[0] & 0xf;

	_response_header_stream.write(header, 1);

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

#include <byteswap.h>

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
		if (payload == PAYLOAD_LENGTH_64)
			_leftToRead = be64toh(_leftToRead);
		else
			_leftToRead = ntohs(_leftToRead);
		#endif
	}

	size_t pos_end_header = INITIAL_PAYLOAD_LEN + MASKING_KEY_LEN + payloadLenBytes;
	std::cout << "payloadLength : " << _leftToRead << std::endl;

	std::memcpy(_maskingKey, header + INITIAL_PAYLOAD_LEN + payloadLenBytes, MASKING_KEY_LEN);
	return pos_end_header;
}

int WebSocketHandler::writeToBody(std::stringstream &bodyStream, char *buffer, const ssize_t &nbytes, u_int64_t &leftToRead) {

	bool first = true;
	for (size_t i = _byte; i < _byte + nbytes; i++) {
		const char unmaskedByte = buffer[i - _byte] ^ _maskingKey[i % 4];
		//_response_body_stream.write(&unmaskedByte, sizeof(unmaskedByte));
		bodyStream.write(&unmaskedByte, sizeof(unmaskedByte));
		if (bodyStream.fail())
			throw std::runtime_error("writing to request body stream");
	}
	_byte += nbytes;
	leftToRead -= nbytes;
	return leftToRead > 0;
}

bool WebSocketHandler::hasBodyExceeded() const {
	return false;
}

int WebSocketHandler::parseRequest(std::stringstream &headerStream) {
	return _leftToRead;
}

void WebSocketHandler::createHttpResponse(std::stringstream &bodyStream) {
	std::string request_body = bodyStream.str();

	u_int64_t response_body_len = request_body.length();

	//std::cout << _response_body_stream.str() << std::endl;

	_response_body_stream.write(request_body.c_str(), request_body.size());

	u_int32_t payload_bytes;
	u_int8_t l;
	if (response_body_len <= 125) {
		_response_header_stream.write(reinterpret_cast<const char*>(&response_body_len), 1);
		return ;
	}
	else if (response_body_len <= 65535) {
		l = 126;
		payload_bytes = 2;
	}
	else {
		l = 127;
		payload_bytes = 8;
	}

	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	if (response_body_len > 65535)
		response_body_len = htobe64(response_body_len);
	else
		response_body_len = htons(response_body_len);
	#endif

	_response_header_stream.write(reinterpret_cast<const char*>(&l), 1);
	_response_header_stream.write(reinterpret_cast<const char*>(&response_body_len), payload_bytes);
}


void WebSocketHandler::writeHeaderStream() {

}

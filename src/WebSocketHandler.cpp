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
	_byte = 0;
	_opcode = 0;
	_fin = header[0] >> 7 & 1;
    _rsv1 = header[0] >> 6 & 1;
    _rsv2 = header[0] >> 5 & 1;
    _rsv3 = header[0] >> 4 & 1;
	_opcode = header[0] & 0xf;

	if (_opcode == OPCODE_PING) {
		header[0] |= OPCODE_PONG;
		header[0] &= 0xfa;
	}

	_response_header_stream.write(header, 1);
}

bool WebSocketHandler::IsKeepAlive() const {
	return _opcode != OPCODE_CLOSE;
}

void WebSocketHandler::ResetRequestContext() {
	_response_body_stream.str(std::string());
	_response_body_stream.clear();
	_response_header_stream.str(std::string());
	_response_header_stream.clear();
}

#include <byteswap.h>

size_t WebSocketHandler::GetPositionEndHeader(char *header) {
	// if < 2 : return std:string::npos
	u_int8_t payload = header[1] & 0x7f;
    uint8_t _payloadBytes = 0;
	_leftToRead = payload;
	if (payload == PAYLOAD_LENGTH_16)
		_payloadBytes = 2;
	else if (payload == PAYLOAD_LENGTH_64)
		_payloadBytes = 8;
	if (_payloadBytes) {
		memcpy(&_leftToRead, header + 2, _payloadBytes);
		#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		if (payload == PAYLOAD_LENGTH_64)
			_leftToRead = be64toh(_leftToRead);
		else
			_leftToRead = ntohs(_leftToRead);
		#endif
	}

	size_t pos_end_header = INITIAL_PAYLOAD_LEN + MASKING_KEY_LEN + _payloadBytes;
	std::cout << "payloadLength : " << _leftToRead << std::endl;

	std::memcpy(_maskingKey, header + INITIAL_PAYLOAD_LEN + _payloadBytes, MASKING_KEY_LEN);
	return pos_end_header;
}

int WebSocketHandler::WriteToBody(std::stringstream &bodyStream, char *buffer, const ssize_t &nbytes, u_int64_t &leftToRead) {
	for (size_t i = _byte; i < _byte + nbytes; i++) {
		const char unmaskedByte = buffer[i - _byte] ^ _maskingKey[i % 4];
		bodyStream.write(&unmaskedByte, sizeof(unmaskedByte));
		if (bodyStream.fail())
			throw std::runtime_error("writing to request body stream");
	}
	_byte += nbytes;
	leftToRead -= nbytes;
	return leftToRead > 0;
}

bool WebSocketHandler::HasBodyExceeded() const {
	return false;
}

int WebSocketHandler::ParseRequest(std::stringstream &headerStream) {
	return _leftToRead;
}

void WebSocketHandler::CreateHttpResponse(std::stringstream &bodyStream) {

	std::string request_body = bodyStream.str();
	u_int64_t response_body_len = request_body.length();

	_response_body_stream.write(request_body.c_str(), request_body.size());
	u_int32_t payload_bytes = 0;
	u_int8_t l = response_body_len;


    if (response_body_len <= MEDIUM_THRESHOLD) {
		l = 126;
		payload_bytes = 2;
	}
	else {
		l = 127;
		payload_bytes = 8;
	}

	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	if (response_body_len > MEDIUM_THRESHOLD)
		response_body_len = htobe64(response_body_len);
	else
		response_body_len = htons(response_body_len);
	#endif

	_response_header_stream.write(reinterpret_cast<const char*>(&l), 1);

    if (response_body_len > SMALL_THRESHOLD)
	    _response_header_stream.write(reinterpret_cast<const char*>(&response_body_len), payload_bytes);
}


void WebSocketHandler::WriteHeaderStream() {

}

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

WebSocketHandler::WebSocketHandler() {}

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

size_t WebSocketHandler::GetPositionEndHeader(char *header) {
	uint8_t payload = header[1] & 0x7f;
    uint8_t payloadBytes = 0;
	_leftToRead = payload;
	if (payload == PAYLOAD_LENGTH_16)
		payloadBytes = 2;
	else if (payload == PAYLOAD_LENGTH_64)
		payloadBytes = 8;
	if (payloadBytes) {
		memcpy(&_leftToRead, header + 2, payloadBytes);
		#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		_leftToRead = payload == PAYLOAD_LENGTH_64 ? be64toh(_leftToRead) : ntohs(_leftToRead);
		#endif
	}
	size_t pos_end_header = INITIAL_PAYLOAD_LEN + MASKING_KEY_LEN + payloadBytes;
	std::memcpy(_maskingKey, header + INITIAL_PAYLOAD_LEN + payloadBytes, MASKING_KEY_LEN);
	return pos_end_header;
}

int WebSocketHandler::WriteToBody(std::stringstream &bodyStream, char *buffer, const ssize_t &nbytes) {
	for (size_t i = _byte; i < _byte + nbytes; i++) {
		const char unmaskedByte = buffer[i - _byte] ^ _maskingKey[i % 4];
		bodyStream.write(&unmaskedByte, sizeof(unmaskedByte));
		if (bodyStream.fail())
			throw std::runtime_error("writing to request body stream");
	}
	_byte += nbytes;
    return nbytes;
}

bool WebSocketHandler::HasBodyExceeded() const {
	return false;
}

int WebSocketHandler::ParseRequest(std::stringstream &headerStream) {
	return _leftToRead;
}

void WebSocketHandler::CreateHttpResponse(std::stringstream &bodyStream) {
	std::string request_body = bodyStream.str();
	uint64_t body_len = request_body.length();

	_response_body_stream.write(request_body.c_str(), body_len);
	uint32_t payload_bytes = 0;
	uint8_t l = body_len;

    if (body_len <= MEDIUM_THRESHOLD) {
		l = 126;
		payload_bytes = 2;
	}
	else {
		l = 127;
		payload_bytes = 8;
	}

	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	body_len = body_len > MEDIUM_THRESHOLD ? htobe64(body_len) : htons(body_len);
	#endif

	_response_header_stream.write(reinterpret_cast<const char*>(&l), 1);
    if (body_len > SMALL_THRESHOLD)
	    _response_header_stream.write(reinterpret_cast<const char*>(&body_len), payload_bytes);
}

void WebSocketHandler::WriteHeaderStream() {}

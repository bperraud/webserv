#ifndef WEBSOCKETHANDLER_HPP
#define WEBSOCKETHANDLER_HPP

#include "ProtocolHandlerInterface.hpp"
#include <string>
#include <cstring>
#include <iostream>
#include <map>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <arpa/inet.h>
#include <byteswap.h>

//#include "bitset"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define OPCODE_CONT 0x0
#define OPCODE_TEXT 0x1
#define OPCODE_BINARY 0x2
#define OPCODE_CLOSE 0x8
#define OPCODE_PING 0x9
#define OPCODE_PONG 0xa

//    # available operation code value tuple
//    OPCODES = (OPCODE_CONT, OPCODE_TEXT, OPCODE_BINARY, OPCODE_CLOSE,
//               OPCODE_PING, OPCODE_PONG)

//    # data length threshold.
//    LENGTH_7 = 0x7e
//    LENGTH_16 = 1 << 16
//    LENGTH_63 = 1 << 63

//header = self.recv_strict(2)
//        b1 = header[0]
//        fin = b1 >> 7 & 1
//        rsv1 = b1 >> 6 & 1
//        rsv2 = b1 >> 5 & 1
//        rsv3 = b1 >> 4 & 1
//        opcode = b1 & 0xf
//        b2 = header[1]
//        has_mask = b2 >> 7 & 1
//        length_bits = b2 & 0x7f

#define MASKING_KEY_LEN 4
#define INITIAL_PAYLOAD_LEN 2
#define PAYLOAD_LENGTH_16 126
#define PAYLOAD_LENGTH_64 127
#define SMALL_THRESHOLD 125
#define MEDIUM_THRESHOLD 65535

class WebSocketHandler : public ProtocolHandlerInterface {

private:

	bool		_fin;
	bool		_rsv1;
	bool		_rsv2;
	bool		_rsv3;
	uint8_t 	_opcode;

	char		_maskingKey[4];

	uint64_t	_byte;
	uint64_t	_leftToRead;

	static const std::map<int, std::string>	_OPCODE_MAP;

public:
    WebSocketHandler();
	WebSocketHandler(char *header);

	size_t	GetPositionEndHeader(char *header) override;

	int 	WriteToBody(std::stringstream &bodyStream, char* buffer,
						const ssize_t &nbytes) override;

	bool	HasBodyExceeded() const;
	bool	IsKeepAlive() const;

	void	CreateHttpResponse(std::stringstream &bodyStream);
	void	CreateHttpResponse(char * buffer, uint64_t size);
	void	ResetRequestContext();
	int		ParseRequest(std::stringstream &_readStream);

	void Handshake(const std::string &webSocketKey);
	void WriteHeaderStream();
};

#endif

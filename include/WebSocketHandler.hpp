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


class WebSocketHandler : public ProtocolHandlerInterface {

private:

	bool		_fin;
	bool		_rsv1;
	bool		_rsv2;
	bool		_rsv3;
	uint8_t 	_opcode;

	static const std::map<int, std::string>	_OPCODE_MAP;


public:
    WebSocketHandler();
	WebSocketHandler(char *header);

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;

	bool	hasBodyExceeded() const;
	bool 	isBodyFinished(std::stringstream &bodyStream, uint64_t &leftToRead, ssize_t nbytes);
	bool	isKeepAlive() const;

	bool 	bodyExceeded(std::stringstream &bodyStream, ssize_t nbytes);
	void	createHttpResponse(std::stringstream &bodyStream);


	void	resetRequestContext();
	int		parseRequest(std::stringstream &_readStream);

	void handshake(const std::string &webSocketKey);
	void writeHeaderStream();
};

#endif

#ifndef WEBSOCKETHANDLER_HPP
#define WEBSOCKETHANDLER_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

# define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

//OPCODE_CONT = 0x0
//    OPCODE_TEXT = 0x1
//    OPCODE_BINARY = 0x2
//    OPCODE_CLOSE = 0x8
//    OPCODE_PING = 0x9
//    OPCODE_PONG = 0xa

//    # available operation code value tuple
//    OPCODES = (OPCODE_CONT, OPCODE_TEXT, OPCODE_BINARY, OPCODE_CLOSE,
//               OPCODE_PING, OPCODE_PONG)

//    # opcode human readable string
//    OPCODE_MAP = {
//        OPCODE_CONT: "cont",
//        OPCODE_TEXT: "text",
//        OPCODE_BINARY: "binary",
//        OPCODE_CLOSE: "close",
//        OPCODE_PING: "ping",
//        OPCODE_PONG: "pong"
//    }

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

struct HttpResponse;

class WebSocketHandler {

private:
	//HttpResponse	&_response;

public:
    WebSocketHandler();
    WebSocketHandler(HttpResponse &response);

	void upgradeWebsocket(const std::string &webSocketKey);
};

#endif

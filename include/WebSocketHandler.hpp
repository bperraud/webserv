#ifndef WEBSOCKETHANDLER_HPP
#define WEBSOCKETHANDLER_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
//#include "HttpHandler.hpp"

# define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

struct HttpResponse;

class WebSocketHandler {

private:
	HttpResponse	&_response;

public:
    WebSocketHandler() = default;
    WebSocketHandler(HttpResponse &response);

	void upgradeWebsocket(const std::string &webSocketKey);
};

#endif

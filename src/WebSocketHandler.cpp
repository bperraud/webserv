#include "WebSocketHandler.hpp"
#include "HttpHandler.hpp"

WebSocketHandler::WebSocketHandler(HttpResponse &response) : _response(response)
{

}

void WebSocketHandler::upgradeWebsocket(const std::string &webSocketKey) {

	std::string salt = webSocketKey + GUID;
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(), sha1_hash);

    BIO *bio, *b64;
    BUF_MEM *bptr;

    bio = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_write(bio, sha1_hash, SHA_DIGEST_LENGTH);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bptr);

    std::string encoded(reinterpret_cast<char*>(bptr->data), bptr->length - 1);
    BIO_free_all(bio);

	_response.map_headers["Connection"] = "Upgrade";
	_response.map_headers["Upgrade"] = "websocket";
	_response.map_headers["Sec-WebSocket-Accept"] = encoded;
}

#include "WebSocketHandler.hpp"
#include "HttpHandler.hpp"
#include "bitset"

const std::map<int, std::string> WebSocketHandler::_OPCODE_MAP = {
	{OPCODE_CONT, "cont"},
    {OPCODE_TEXT, "text"},
    {OPCODE_BINARY, "binary"},
    {OPCODE_CLOSE, "close"},
    {OPCODE_PING, "ping"},
    {OPCODE_PONG, "pong"}
};

WebSocketHandler::WebSocketHandler()
{

}

WebSocketHandler::WebSocketHandler(char *header)
{
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

void WebSocketHandler::resetRequestContext()
{
	_response_body_stream.str(std::string());
	_response_body_stream.clear();
	_response_header_stream.str(std::string());
	_response_header_stream.clear();
}

bool WebSocketHandler::hasBodyExceeded() const
{
	return false;
}

bool WebSocketHandler::isBodyFinished(std::stringstream &bodyStream, uint64_t &leftToRead, ssize_t nbytes)
{
	return false;
}

bool WebSocketHandler::bodyExceeded(std::stringstream &bodyStream, ssize_t nbytes)
{
	return false;
}

void WebSocketHandler::createHttpResponse(std::stringstream &bodyStream)
{
}

int WebSocketHandler::parseRequest(std::stringstream &_readStream)
{
	return 0;
}

void WebSocketHandler::writeHeaderStream()
{

}

#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <functional>

#include "Utils.hpp"
#include "ServerConfig.hpp"
#include "ErrorHandler.hpp"
#include "CGIExecutor.hpp"
#include "WebSocketHandler.hpp"
#include "ProtocolHandlerInterface.hpp"

# define EOF_CHUNKED "\r\n0\r\n\r\n"
# define CRLF "\r\n\r\n"
# define ROOT_PATH "www"
# define OVERLAP 4

struct server;

typedef std::map<std::string, server>	server_name_level3;

struct HttpMessage {
    std::string method;
    std::string url;
    std::string version;
	std::string host;
    std::map<std::string, std::string> map_headers;
    size_t bodyLength;
};

struct HttpResponse {
    std::string version;
    std::string status_code;
    std::string statusPhrase;
    std::map<std::string, std::string> map_headers;
};


class HttpHandler : public ProtocolHandlerInterface {

private:

	bool				_keepAlive;
	bool				_bodySizeExceeded;
	bool				_transferChunked;
	bool				_isWebSocket;

	std::string			_request_body;

	std::stringstream   _response_header_stream;
	std::stringstream   _response_body_stream;
	HttpMessage			_request;
	HttpResponse		_response;

	static const std::map<std::string, std::string>				_MIME_TYPES;
	static const std::map<int, std::string>						_SUCCESS_STATUS;
	static const std::map<std::string, void (HttpHandler::*)()>	_HTTP_METHOD;

	server_name_level3*	_serverMap;
	server_config*		_server;
	routes				_default_route;
	routes*				_active_route;

private:

	void	assignServerConfig();
	void	createStatusResponse(int code);
	void	uploadFile(const std::string& contentType, size_t pos_boundary);
	void 	redirection();
	void	unchunckMessage(std::stringstream &bodyStream);

	std::string		getHeaderValue(const std::string &header) const;
	std::string		getContentType(const std::string& path) const;
	bool	invalidRequestLine() const;
	void	GET();
	void	DELETE();
	void	POST();
	void	setupRoute(const std::string &url);
	bool	isAllowedMethod(const std::string &method) const;
	void	constructStringResponse();
	void	generateDirectoryListing(const std::string& directory_path);
	void	handleCGI(const std::string &original_url);
	void	error(int error);
	void	handshake(const std::string &webSocketKey);

public:
	HttpHandler(int timeoutSeconds, server_name_level3 *serv_map);
	~HttpHandler();

	bool	hasBodyExceeded() const;
	bool	isKeepAlive() const;
	bool 	isBodyFinished(std::stringstream &bodyStream, uint64_t &leftToRead, ssize_t nbytes);

	bool 	bodyExceeded(std::stringstream &bodyStream, ssize_t nbytes);
	int		transferChunked(std::stringstream &bodyStream);
	void	createHttpResponse(std::stringstream &bodyStream);

	std::string		getResponseHeader() const;
	std::string		getResponseBody() const;

	void	resetRequestContext();
	int		parseRequest(std::stringstream &_readStream);

};

#endif

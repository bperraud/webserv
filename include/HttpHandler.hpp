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

	void	AssignServerConfig();
	void	CreateStatusResponse(int code);
	void	UploadFile(const std::string& contentType, size_t pos_boundary);
	void 	Redirection();
	void	UnchunckMessage(std::stringstream &bodyStream);

	std::string		GetHeaderValue(const std::string &header) const;
	std::string		GetContentType(const std::string& path) const;
	bool	InvalidRequestLine() const;
	void	GET();
	void	DELETE();
	void	POST();
	void	SetupRoute(const std::string &url);
	bool	IsAllowedMethod(const std::string &method) const;
	void	ConstructStringResponse();
	void	GenerateDirectoryListing(const std::string& directory_path);
	void	HandleCGI(const std::string &original_url);
	void	Error(int error);
	void	Handshake(const std::string &webSocketKey);

public:
	HttpHandler(server_name_level3 *serv_map);
	~HttpHandler();

	size_t	GetPositionEndHeader(char *buffer) override;
	bool	HasBodyExceeded() const;
	bool	IsKeepAlive() const;
	bool 	IsBodyFinished(std::stringstream &bodyStream, const ssize_t &nbytes);

	int		WriteToBody(std::stringstream &bodyStream, char* buffer, const ssize_t &nbytes) override;
	bool 	BodyExceeded(std::stringstream &bodyStream, const ssize_t &nbytes);
	int		TransferChunked(std::stringstream &bodyStream);
	void	CreateHttpResponse(std::stringstream &bodyStream);

	void	ResetRequestContext();
	int		ParseRequest(std::stringstream &headerStream);

};

#endif

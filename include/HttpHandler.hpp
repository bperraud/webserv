#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP


#include "ResponseServer.hpp"
#include "RequestClient.hpp"

#include <sys/stat.h> // stat()
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <dirent.h> //closedir()

enum Type {
	GET,
	POST,
	DELETE,
	HEAD
};

#define ROOT_PATH "./website"
#define DEFAULT_PAGE "./website/index.html"

#if 0
Recipients of an invalid request-line SHOULD respond with either a
   400 (Bad Request) error or a 301 (Moved Permanently) redirect with
   the request-target properly encoded
#endif

struct HttpMessage {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> map_headers;
    bool has_body;
    size_t body_length;
    std::stringstream body_stream;
};

struct HttpResponse {
    std::string version;
    std::string status_code;
    std::string status_phrase;
    std::map<std::string, std::string> map_headers;
    std::stringstream body_stream;
	std::stringstream header_stream;
};

class HttpHandler {

private:

	std::stringstream	*_readStream;
	HttpMessage			_request;
	HttpResponse		_response;
	bool				_close_keep_alive;
	int					_type;
	char				_lastFour[4];

	ssize_t				_left_to_read;
	std::map<std::string, std::string> _MIME_TYPES;


	HttpHandler(const HttpHandler &copy) {
		*this = copy;
	}

	HttpHandler &operator=(HttpHandler const &other) {
		if (this != &other) {
			;
		}
		return *this;
	}


public:
	HttpHandler();
	~HttpHandler();

	bool		isKeepAlive() const;

	void	writeToStream(char *buffer, ssize_t nbytes) ;
	int		writeToBody(char *buffer, ssize_t nbytes);

	std::string		getRequest() {
		return _readStream->str();
	}

	std::string		getBody() {
		return _request.body_stream.str();
	}

	std::string		getRequestMethod() {
		return _request.method;
	}

	void	resetStream() {
		delete _readStream;
		_readStream = new std::stringstream();

		_request.body_stream.str(std::string());
		_request.body_stream.seekp(0, std::ios_base::beg);
		_response.body_stream.str(std::string());
		_response.header_stream.str(std::string());
		_lastFour[0] = '\0';
	}

	void	copyLastFour(char *buffer, ssize_t nbytes);

	ssize_t getLeftToRead() const {
		return _left_to_read;
	}

	std::string getResponseHeader() {
		return _response.header_stream.str();
	}

	std::string getResponseBody() {
		return _response.body_stream.str();
	}

	void parseRequest();
	void createHttpResponse();

	void GET();
	void POST();
	void DELETE();

	void constructStringResponse();

	bool pathToFileExist(const std::string& path);
	bool isDirectory(const std::string& path);
	bool pathExists(const std::string& path);

	void addFileToResponse(const std::string &fileName);

	std::string getContentType(const std::string& path);
	std::string intToString(int value);
};

#endif

// no body ? end with \r\n\r\n
// GET and HEAD : no body
// POST and PUT), the presence of the Content-Length header or
// the Transfer-Encoding header indicates the length of the request body

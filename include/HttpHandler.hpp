#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include "Utils.hpp"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "ErrorHandler.hpp"
#include "ServerError.hpp"
#include "ClientError.hpp"

# define CRLF "\r\n\r\n"

enum Type {
	GET,
	POST,
	DELETE,
	HEAD
};

#define ROOT_PATH "./website"
#define DEFAULT_PAGE "./website/index.html"
#define UPLOAD_PATH "./website/upload/"


#if 0
Recipients of an invalid request-line SHOULD respond with either a
   400 (Bad Request) error or a 301 (Moved Permanently) redirect with
   the request-target properly encoded
#endif

struct HttpMessage {
    std::string method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> map_headers;
    bool has_body;
    size_t body_length;
};

struct HttpResponse {
    std::string version;
    std::string status_code;
    std::string status_phrase;
    std::map<std::string, std::string> map_headers;
};

class HttpHandler {

private:

	std::stringstream	*_readStream;

	std::stringstream   _request_body_stream;
	std::stringstream   _response_header_stream;
	std::stringstream   _response_body_stream;

	HttpMessage			_request;
	HttpResponse		_response;
	bool				_close_keep_alive;
	int					_type;
	char				_last_4_char[4];

	ssize_t				_left_to_read;
	std::map<std::string, std::string> _MIME_TYPES;

	bool				_cgiMode;


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

	bool	isKeepAlive() const;

	void	writeToStream(char *buffer, ssize_t nbytes) ;
	int		writeToBody(char *buffer, ssize_t nbytes);


	void error(int error);

	bool	isCGIMode() const {
		return _cgiMode;
	}

	HttpMessage getStructRequest() {
		return _request;
	}

	std::string		getRequest() {
		return _readStream->str();
	}

	std::string		getBody() {
		return _request_body_stream.str();
	}

	std::string		getRequestMethod() {
		return _request.method;
	}

	void	resetStream() {
		delete _readStream;
		_readStream = new std::stringstream();

		_request_body_stream.str(std::string());
		_request_body_stream.seekp(0, std::ios_base::beg);
		_response_body_stream.str(std::string());
		_response_header_stream.str(std::string());
		_last_4_char[0] = '\0';
	}

	void	copyLast4Char(char *buffer, ssize_t nbytes);

	ssize_t getLeftToRead() const {
		return _left_to_read;
	}

	std::string getResponseHeader() {
		return _response_header_stream.str();
	}

	std::string getResponseBody() {
		return _response_body_stream.str();
	}

	void parseRequest();
	void createHttpResponse();

	bool findHeader(const std::string &header, std::string &value);

	void GET();
	void POST();
	void DELETE();

	bool isCGI(const std::string &path);
	void constructStringResponse();

	void uploadFile(const std::string& contentType, size_t pos_boundary);

	std::string getContentType(const std::string& path);
};

#endif

// no body ? end with \r\n\r\n
// GET and HEAD : no body
// POST and PUT), the presence of the Content-Length header or
// the Transfer-Encoding header indicates the length of the request body

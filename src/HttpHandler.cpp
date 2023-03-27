#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _readStream(new std::stringstream()),  _close_keep_alive(false), _type(0)
, _left_to_read(0), _MIME_TYPES(){
	_lastFour[0] = '\0';

	_MIME_TYPES["html"] = "text/html";
    _MIME_TYPES["css"] = "text/css";
    _MIME_TYPES["js"] = "text/javascript";
    _MIME_TYPES["png"] = "image/png";
    _MIME_TYPES["jpg"] = "image/jpeg";
    _MIME_TYPES["gif"] = "image/gif";
    _MIME_TYPES["json"] = "application/json";
	_MIME_TYPES["webmanifest"] = "application/manifest+json";
}

HttpHandler::~HttpHandler() {
	delete _readStream;
}

void	HttpHandler::copyLastFour(char *buffer, ssize_t nbytes) {
	if (_lastFour[0])
		std::memcpy(buffer, _lastFour, 4);
	else
		std::memcpy(buffer, buffer + 4, 4);
	std::memcpy(_lastFour, buffer + nbytes, 4);
}

void HttpHandler::writeToStream(char *buffer, ssize_t nbytes) {
	_readStream->write(buffer, nbytes);
}

int	HttpHandler::writeToBody(char *buffer, ssize_t nbytes) {
	if (!_left_to_read)
		return 0;
	_request.body_stream.write(buffer, nbytes);
	_left_to_read -= nbytes;
	return _left_to_read;
}

bool HttpHandler::isKeepAlive() const {
	return _close_keep_alive;
}

void HttpHandler::parseRequest() {
	// Parse the start-line
	*_readStream >> _request.method >> _request.path >> _request.version;

	// Parse the headers into a hash table
	std::string header_name, header_value;
	while (getline(*_readStream, header_name, ':') && getline(*_readStream, header_value, '\r')) {
		// Remove any leading or trailing whitespace from the header value
		header_value.erase(0, header_value.find_first_not_of(" \r\n\t"));
		header_value.erase(header_value.find_last_not_of(" \r\n\t") + 1, header_value.length());
		header_name.erase(0, header_name.find_first_not_of(" \r\n\t"));
		_request.map_headers[header_name] = header_value;
	}
	_request.body_length = 0;
	std::map<std::string, std::string>::iterator content_length_header = _request.map_headers.find("Content-Length");
	if (content_length_header != _request.map_headers.end()) {
		// Parse the content length header to determine the message body length
		std::stringstream ss(content_length_header->second);
		ss >> _request.body_length;
	}

	_close_keep_alive = _request.map_headers["Connection"] == "keep-alive";
	_left_to_read = _request.body_length;
}

std::string HttpHandler::getContentType(const std::string& path) {
    std::string::size_type dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        // No extension found, assume plain text
        return "text/plain";
    }
    std::string ext = path.substr(dot_pos + 1);
    // Look up the MIME type for the file extension
    std::map<std::string, std::string>::const_iterator it = _MIME_TYPES.find(ext);
    if (it == _MIME_TYPES.end()) {
        // Extension not found, assume plain text
        return "text/plain";
    }
    return it->second;
}

void HttpHandler::createHttpResponse() {
	int index;
	std::string type[4] = {"GET", "POST", "DELETE", ""};

	_response.version = _request.version;
	for (index = 0; index < 4; index++)
	{
		if (type[index].compare(_request.method) == 0)
			break;
	}
	switch (index)
	{
		case 0:
			GET();
			break;
		case 1:
			POST();
			break;
		case 2:
			DELETE();
			break;
		default:
			std::cout << "Wrong Request Type" << std::endl;
			return ;
	}
	constructStringResponse();
}

void HttpHandler::GET() {
	_response.status_code = "200";
	_response.status_phrase = "OK";

	if (!_request.path.compare("/")) {
		Utils::loadFile(DEFAULT_PAGE, _response.body_stream);
		_response.map_headers["Content-Type"] = getContentType(DEFAULT_PAGE);
		_response.map_headers["Content-Length"] = Utils::intToString(_response.body_stream.str().length());
		return ;
	}

	_request.path = ROOT_PATH + _request.path;
	if ( Utils::isDirectory(_request.path))
	{
		;//directory listing
	}
	else if (Utils::pathToFileExist(_request.path)) {
		Utils::loadFile(_request.path, _response.body_stream);
		_response.map_headers["Content-Type"] = getContentType(_request.path);
	}
	else
	{
		_response.status_code = "404";
		_response.status_phrase = "Not Found";
		_response.body_stream << "<html><body><h1>404 Not Found</h1></body></html>";
		_response.map_headers["Content-Type"] = "text/html";
	}

	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = Utils::intToString(_response.body_stream.str().length());
}


void HttpHandler::POST() {
	_response.status_code = "200";
	_response.status_phrase = "OK";

	size_t pos_boundary = _request.map_headers["Content-Type"].find("boundary=");
	if (pos_boundary != std::string::npos) {	//multipart/form-data

		std::string messageBody = _request.body_stream.str();
		std::string boundary = _request.map_headers["Content-Type"].substr(pos_boundary + 9);

		std::string fileContentEnd = "\r\n--" + boundary + "--";
		size_t start = messageBody.find(CRLF);
		size_t end = messageBody.find(fileContentEnd, start);

		if (start == std::string::npos || end == std::string::npos) {
			_response.status_code = "400";
			_response.status_phrase = "Bad Request";
			Utils::loadFile("./website/400.html", _response.body_stream);
			_response.map_headers["Location"] = "http://localhost:8080/400.html";
			_response.map_headers["Content-Type"] = getContentType("./website/400.html");
			_response.map_headers["Content-Length"] = Utils::intToString(_response.body_stream.str().length());
        	return ;
    	}

		start += std::strlen(CRLF);
    	messageBody =  messageBody.substr(start, end - start);

		_response.status_code = "201";
		_response.status_phrase = "Created";

		return ;
		//std::ofstream *outfile = Utils::createOrEraseFile("upload.html");
		// Write the data to the file
		//_response.body_stream.write(start, end - start);
		//return ;
	}

	else if (_request.map_headers["Content-Type"].find("application/x-www-form-urlencoded") != std::string::npos) {
		//application/x-www-form-urlencoded

	}


	#if 0
	_response.version = _request.version;
	_response.status_code = "302";
	_response.status_phrase = "Found";

	//addFileToResponse(DEFAULT_PAGE);
	_response.map_headers["Location"] = "http://localhost:8080/index.html";
	#endif
	//_response.map_headers["Content-Type"] = getContentType(DEFAULT_PAGE);
	//_response.map_headers["Content-Length"] = Utils::intToString(_response.body_stream.str().length());
}


void HttpHandler::DELETE() {
	;
}

void HttpHandler::constructStringResponse() {
	bool first = true;
	_response.header_stream << _response.version << " " << _response.status_code << " " << _response.status_phrase << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _response.map_headers.begin(); it != _response.map_headers.end(); ++it) {
		if (!first)
			_response.header_stream << "\r\n";
		_response.header_stream << it->first << ": " << it->second;
		first = false;
	}
	_response.header_stream << "\r\n\r\n";
}

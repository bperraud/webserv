#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _readStream(new std::stringstream()),  _close_keep_alive(false), _type(0)
, _left_to_read(0), _MIME_TYPES(), _cgiMode(false){
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
	_request_body_stream.write(buffer, nbytes);
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

bool HttpHandler::isCGI(const std::string &path) {
	return path.substr(0, std::strlen("/cgi-bin/")).compare("/cgi-bin/") == 0;
}

void HttpHandler::error(int error) {
	ErrorHandler* error_handler;
	if (error >= 500)
		error_handler = new ServerError(_response, _response_body_stream);
	else
		error_handler = new ClientError(_response, _response_body_stream);
	error_handler->errorProcess(error);
	delete error_handler;
}

void HttpHandler::GET() {
	_response.status_code = "200";
	_response.status_phrase = "OK";

	if (!_request.path.compare("/")) {
		Utils::loadFile(DEFAULT_PAGE, _response_body_stream);
		_response.map_headers["Content-Type"] = getContentType(DEFAULT_PAGE);
		_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
		return ;
	}
	else if (isCGI(_request.path))
	{
		_cgiMode = true;
		return ;
	}

	_request.path = ROOT_PATH + _request.path;
	if ( Utils::isDirectory(_request.path))
	{
		;//directory listing
	}
	else if (Utils::pathToFileExist(_request.path)) {
		Utils::loadFile(_request.path, _response_body_stream);
		_response.map_headers["Content-Type"] = getContentType(_request.path);
	}
	else
	{
		error(404);
		return;
	}
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
}

bool HttpHandler::findHeader(const std::string &header, std::string &value) {
	std::map<std::string, std::string>::iterator it = _request.map_headers.find(header);
	if (it != _request.map_headers.end()) {
		value = it->second;
		return true;
	}
	return false;
}

		//if (!findHeader("Content-Disposition", extractContent)) {
		//	std::cout << "Content-Disposition not found" << std::endl;
		//	return ;
		//}

void HttpHandler::POST() {
	_response.status_code = "200";
	_response.status_phrase = "OK";

	std::string contentType;
	if (!findHeader("Content-Type", contentType))
		return error(400);

	size_t pos_boundary = contentType.find("boundary=");
	if (pos_boundary != std::string::npos) {	//multipart/form-data

		std::string fileName;
		std::string messageBody = _request_body_stream.str();
		std::string boundary = contentType.substr(pos_boundary + 9);
		size_t pos1 = messageBody.find("filename=\"");
		if (pos1 != std::string::npos) {
			// The filename is enclosed in double quotes
			size_t pos2 = messageBody.find("\"", pos1 + 10);
			if (pos2 != std::string::npos)
				fileName = messageBody.substr(pos1 + 10, pos2 - pos1 - 10);
			else
				return error(400);
		}
		else
			return error(400);

		size_t start = messageBody.find(CRLF);
		size_t end = messageBody.find("\r\n--" + boundary + "--", start);
		if (start == std::string::npos || end == std::string::npos)
			return error(400);

		start += std::strlen(CRLF);
    	messageBody =  messageBody.substr(start, end - start);

		std::ofstream *outfile = Utils::createOrEraseFile(fileName.c_str());
		outfile->write(messageBody.c_str(), messageBody.length());
    	outfile->close();

		_response.status_code = "201";
		_response.status_phrase = "Created";

		_response_body_stream << "<html><head><title>Upload</title></head><body>\n";

		_response.map_headers["Content-Type"] = getContentType(fileName);
		_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());

		delete outfile;
		return ;
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
	//_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
}


void HttpHandler::DELETE() {
	;
}

void HttpHandler::constructStringResponse() {
	bool first = true;
	_response_header_stream << _response.version << " " << _response.status_code << " " << _response.status_phrase << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _response.map_headers.begin(); it != _response.map_headers.end(); ++it) {
		if (!first)
			_response_header_stream << "\r\n";
		_response_header_stream << it->first << ": " << it->second;
		first = false;
	}
	_response_header_stream << "\r\n\r\n";
}

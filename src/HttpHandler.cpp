#include "HttpHandler.hpp"


HttpHandler::HttpHandler(int timeout_seconds = 5) : _timer(timeout_seconds), _readStream(new std::stringstream()),  _close_keep_alive(false), _type(0)
, _left_to_read(0), _MIME_TYPES(), _cgiMode(false){
	_last_4_char[0] = '\0';
	_MIME_TYPES["html"] = "text/html";
    _MIME_TYPES["css"] = "text/css";
    _MIME_TYPES["js"] = "text/javascript";
    _MIME_TYPES["png"] = "image/png";
    _MIME_TYPES["jpg"] = "image/jpeg";
    _MIME_TYPES["gif"] = "image/gif";
    _MIME_TYPES["json"] = "application/json";
	_MIME_TYPES["webmanifest"] = "application/manifest+json";
	_MIME_TYPES["ico"] = "image/x-icon";
	_MIME_TYPES["txt"] = "text/plain";
	_MIME_TYPES["pdf"] = "application/pdf";
	_MIME_TYPES["mp3"] = "audio/mpeg";
	_MIME_TYPES["mp4"] = "video/mp4";
	_MIME_TYPES["mpeg"] = "video/mpeg";
	_MIME_TYPES["mov"] = "video/quicktime";
	_MIME_TYPES["zip"] = "application/zip";
	_MIME_TYPES["gz"] = "application/gzip";
	_MIME_TYPES["tar"] = "application/x-tar";
	_MIME_TYPES["xml"] = "application/xml";
	_MIME_TYPES["wav"] = "audio/x-wav";
	_MIME_TYPES["bmp"] = "image/bmp";
	_MIME_TYPES["webp"] = "image/webp";
	_MIME_TYPES["doc"] = "application/msword";
	_MIME_TYPES["php"] = "text/html";
	_MIME_TYPES["py"] = "text/html";
}

HttpHandler::~HttpHandler() {
	delete _readStream;
}

void	HttpHandler::copyLast4Char(char *buffer, ssize_t nbytes) {
	if (_last_4_char[0])
		std::memcpy(buffer, _last_4_char, 4);
	else
		std::memcpy(buffer, buffer + 4, 4);
	std::memcpy(_last_4_char, buffer + nbytes, 4);
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
	*_readStream >> _request.method >> _request.url >> _request.version;
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
	std::string content_length_header;
	if (findHeader("Content-Length", content_length_header)) {
		std::stringstream ss(content_length_header);
		ss >> _request.body_length;
	}
	std::string connection_header;
	if (findHeader("Connection", connection_header))
		_close_keep_alive = connection_header == "keep-alive";
	else
		_close_keep_alive = true;
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
			return error(405);
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

	if (!_request.url.compare("/")) {
		Utils::loadFile(DEFAULT_PAGE, _response_body_stream);
		_response.map_headers["Content-Type"] = getContentType(DEFAULT_PAGE);
		_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
		return ;
	}
	else if (isCGI(_request.url))
	{
		_cgiMode = true;
		return ;
	}

	_request.url = ROOT_PATH + _request.url;
	if ( Utils::isDirectory(_request.url))
	{
		;//directory listing
	}
	else if (Utils::pathToFileExist(_request.url)) {
		Utils::loadFile(_request.url, _response_body_stream);
		_response.map_headers["Content-Type"] = getContentType(_request.url);
	}
	else
		return error(404);
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

void HttpHandler::uploadFile(const std::string& contentType, size_t pos_boundary) {
	std::string messageBody = _request_body_stream.str();
	std::string boundary = contentType.substr(pos_boundary + 9);
	const size_t headerPrefixLength = strlen("filename=\"");
	size_t pos1 = messageBody.find("filename=\"");
	size_t pos2 = pos1 != std::string::npos ? messageBody.find("\"", pos1 + headerPrefixLength) : std::string::npos;
	std::string fileName = pos2 != std::string::npos ? messageBody.substr(pos1 + headerPrefixLength, pos2 - pos1 - headerPrefixLength) : "";
	if (pos1 == std::string::npos || pos2 == std::string::npos || fileName.empty()) {
		return error(400);
	}
	size_t start = messageBody.find(CRLF);
	size_t end = messageBody.find("\r\n--" + boundary + "--", start);
	if (start == std::string::npos || end == std::string::npos)
		return error(400);
	start += std::strlen(CRLF);
    messageBody =  messageBody.substr(start, end - start);
	std::ofstream *outfile = Utils::createOrEraseFile(UPLOAD_PATH + fileName);
	outfile->write(messageBody.c_str(), messageBody.length());
    outfile->close();
	delete outfile;
	_response.status_code = "201";
	_response.status_phrase = "Created";
	_response_body_stream << messageBody;
	_response.map_headers["Content-Type"] = getContentType(fileName);
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
}

void HttpHandler::POST() {
	_response.status_code = "201";
	_response.status_phrase = "Created";

	std::string contentType;
	if (!findHeader("Content-Type", contentType))
		return error(400);

	size_t pos_boundary = contentType.find("boundary=");
	if (pos_boundary != std::string::npos) {	//multipart/form-data
		uploadFile(contentType, pos_boundary);
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

}


void HttpHandler::DELETE() {
	_response.status_code = "204";
	_response.status_phrase = "No Content";

	std::string file_path = UPLOAD_PATH + _request.url.substr(1);
	std::string decoded_file_path = Utils::urlDecode(file_path);
	if (!Utils::pathToFileExist(decoded_file_path))
		return error(404);
	if (std::remove(decoded_file_path.c_str()) != 0)
		return error(403);
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

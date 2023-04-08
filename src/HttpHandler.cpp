#include "HttpHandler.hpp"


HttpHandler::HttpHandler(int timeout_seconds, const server_config* serv) : _timer(timeout_seconds),
	_readStream(new std::stringstream()),  _close_keep_alive(false),
	_left_to_read(0), _MIME_TYPES(), _cgiMode(false), _ready_to_write(false), _server(*serv),
	_body_size_exceeded(false), _active_route(NULL), _default_route(){
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
	_MIME_TYPES["mov"] = "video/quicktime";
	_MIME_TYPES["xml"] = "application/xml";
	_MIME_TYPES["wav"] = "audio/x-wav";
	_MIME_TYPES["webp"] = "image/webp";
	_MIME_TYPES["doc"] = "application/msword";
	_MIME_TYPES["php"] = "text/html";
	_MIME_TYPES["py"] = "text/html";

	_default_route.index = "index.html";
	_default_route.autoindex = false;
	_default_route.methods[0] = "GET";
	_default_route.methods[1] = "DELETE";
	_default_route.methods[2] = "POST";
	_default_route.root = "./website";
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
	if ( _server.max_body_size && static_cast<ssize_t>(_request_body_stream.tellp()) + nbytes > _server.max_body_size) {
		_left_to_read = 0;
		_body_size_exceeded = true;
		return 0;
	}
	_request_body_stream.write(buffer, nbytes);
	_left_to_read -= nbytes;
	return _left_to_read;
}

void HttpHandler::resetStream() {
	delete _readStream;
	_readStream = new std::stringstream();
	_request_body_stream.str(std::string());
	_request_body_stream.seekp(0, std::ios_base::beg);
	_response_body_stream.str(std::string());
	_response_header_stream.str(std::string());
	_last_4_char[0] = '\0';
	_active_route = &_default_route;
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

std::string HttpHandler::getContentType(const std::string& path) const {
    std::string::size_type dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) { // no extension, assume directory
		if (_active_route->autoindex)
			return "text/html";
        return "text/plain";
    }
    std::string ext = path.substr(dot_pos + 1);
    std::map<std::string, std::string>::const_iterator it = _MIME_TYPES.find(ext);
    if (it == _MIME_TYPES.end()) {
        return "";
    }
    return it->second;
}

//bool HttpHandler::isMethodAllowed(const std::string& method) const {
//	std::vector<std::string>::const_iterator it = _server.routes..begin();
//	for (; it != _server.allowed_methods.end(); ++it) {
//		if (*it == method) {
//			return true;
//		}
//	}
//	return false;
//}

bool HttpHandler::correctPath(const std::string& path) const {
	return Utils::isDirectory(ROOT_PATH + path) || Utils::pathToFileExist(ROOT_PATH + path);
}

void HttpHandler::findRoute(const std::string &url) {
	std::map<std::string, routes>::iterator it = _server.routes_map.begin();
	for (; it != _server.routes_map.end(); ++it) {
		if ((url.find(it->first) == 0 && it->first != "/" )|| (url == "/" && it->first == "/")) {
			_active_route = &it->second;
			_request.url = _request.url.replace(url.find(it->first), it->first.length(), it->second.root);
			std::cout << _request.url << std::endl;
			break;
		}
	}
}

bool HttpHandler::isAllowedMethod(const std::string &method) const {
	if (!_active_route)
		return true;
	for (size_t i = 0; i < _active_route->methods->length(); i++) {
		std::cout << _active_route->methods[i] << std::endl;
		if (_active_route->methods[i] == method)
			return true;
	}
	return false;
}

void HttpHandler::createHttpResponse() {
	int index;
	std::string type[4] = {"GET", "POST", "DELETE", ""};
	_response.version = _request.version;

	findRoute(_request.url);
	std::cout << _request.url << std::endl;
	if (!correctPath(_request.url) && !_active_route) {
		error(404);
	}
	else if (_body_size_exceeded) {
		_body_size_exceeded = false;
		resetStream();
		error(413);
	}
	else {
		for (index = 0; index < 4; index++)
		{
			if (type[index].compare(_request.method) == 0 && isAllowedMethod(_request.method))
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
				error(405);
		}
	}
	constructStringResponse();
}

bool HttpHandler::isCGI(const std::string &path) const {
	return path.substr(0, std::strlen("/cgi-bin/")).compare("/cgi-bin/") == 0;
}

void HttpHandler::error(int error) {
	resetStream();
	ErrorHandler* error_handler;
	if (error >= 500)
		error_handler = new ServerError(_response, _response_body_stream);
	else
		error_handler = new ClientError(_response, _response_body_stream);
	error_handler->errorProcess(error);
	delete error_handler;
}

void HttpHandler::generate_directory_listing_html(const std::string& directory_path) {
    // Open the directory
    DIR* dir = opendir(directory_path.c_str());
    if (dir == NULL) {
        std::cerr << "Error: Failed to open directory " << directory_path << std::endl;
        return;
    }
    // Generate the directory listing HTML
    _response_body_stream << "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><table>";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        std::string path = directory_path + "/" + name;

        // Get the file size (if applicable)
        std::string size_str;
        if (entry->d_type == DT_REG) {
            struct stat st;
            if (stat(path.c_str(), &st) == 0) {
                size_str = std::to_string(st.st_size) + " bytes";
            }
        } else {
            size_str = "-";
        }

        // Format the directory entry as an HTML table row
        std::string row = "<tr><td><a href=\"" + name + "\">" + name + "</a></td><td>" + size_str + "</td></tr>";
        _response_body_stream << row;
    }
    _response_body_stream << "</table></body></html>";
    // Close the directory
    closedir(dir);
}


void HttpHandler::GET() {
	_response.status_code = "200";
	_response.status_phrase = "OK";

	if (isCGI(_request.url))
	{
		_cgiMode = true;
		return ;
	}
	if (Utils::isDirectory(_request.url)) { // directory
		if (_active_route->autoindex == true)
		{
			generate_directory_listing_html(_request.url);
		}
		else {
			if (_active_route->index == "") return error(404);	// no index file
			_request.url += "/" + _active_route->index;
			Utils::loadFile(_request.url, _response_body_stream);
		}
	}
	else if (Utils::pathToFileExist(_request.url)) { // file
		Utils::loadFile(_request.url, _response_body_stream);
	}
	else
		return error(404);

	std::string content_type = getContentType(_request.url);
	if (content_type.empty())
		return error(415);
	_response.map_headers["Content-Type"] = content_type;
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
	std::string content_type = getContentType(_request.url);
	if (content_type.empty())
		return error(415);
	else
		_response.map_headers["Content-Type"] = content_type;
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
	_response.map_headers["Location"] = UPLOAD_PATH + fileName;
}

void HttpHandler::POST() {
	_response.status_code = "200";
	_response.status_phrase = "OK";

	std::string contentType;
	if (!findHeader("Content-Type", contentType))
		return error(400);
	size_t pos_boundary = contentType.find("boundary=");
	if (pos_boundary != std::string::npos) {	//multipart/form-data
		uploadFile(contentType, pos_boundary);
		_response.status_code = "201";
		_response.status_phrase = "Created";
	}
	else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
		_response.map_headers["Content-Length"] = "0";
	}
	else { // others
		return error(501);
	}
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

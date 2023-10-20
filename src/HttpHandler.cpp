#include "HttpHandler.hpp"
#include "ServerManager.hpp"

const std::map<std::string, std::string> HttpHandler::_MIME_TYPES = {
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"gif", "image/gif"},
    {"json", "application/json"},
    {"webmanifest", "application/manifest+json"},
    {"ico", "image/x-icon"},
    {"txt", "text/plain"},
    {"pdf", "application/pdf"},
    {"mp3", "audio/mpeg"},
    {"mp4", "video/mp4"},
    {"mov", "video/quicktime"},
    {"xml", "application/xml"},
    {"wav", "audio/x-wav"},
    {"webp", "image/webp"},
    {"doc", "application/msword"},
    {"php", "text/html"},
    {"py", "text/html"}
};

const std::map<int, std::string> HttpHandler::_SUCCESS_STATUS = {
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {209, "IM Used"},
	{301, "Moved Permanently"}
};

HttpHandler::HttpHandler(int timeout_seconds, server_name_level3 *serv_map) : _timer(timeout_seconds),
																			  _readStream(), _request_body_stream(), _response_header_stream(), _response_body_stream(), _leftToRead(0),
																			  _serverMap(serv_map), _server(NULL),
																			  _keepAlive(false), _body_size_exceeded(false), _transfer_chunked(false),
																			  _default_route(), _active_route(&_default_route)
{
	//_overlapBuffer[0] = '\0';
	_default_route.index = "";
	_default_route.autoindex = false;
	_default_route.methods[0] = "GET";
	_default_route.methods[1] = "POST";
	_default_route.methods[2] = "";
	_default_route.root = ROOT_PATH;
}

HttpHandler::~HttpHandler()
{

}

// --------------------------------- GETTERS --------------------------------- //

std::string HttpHandler::getResponseHeader() const { return _response_header_stream.str(); }
std::string HttpHandler::getResponseBody() const { return _response_body_stream.str(); }
bool HttpHandler::isKeepAlive() const { return _keepAlive; }
//bool HttpHandler::isReadyToWrite() const { return _ready_to_write; }

std::string HttpHandler::getContentType(const std::string &path) const
{
	std::string::size_type dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos) // no extension, assume directory
		return _active_route->autoindex ?  "text/html" : "text/plain";
	std::map<std::string, std::string>::const_iterator it = _MIME_TYPES.find(path.substr(dot_pos + 1));
	return it == _MIME_TYPES.end() ? "" : it->second;
}

bool HttpHandler::isBodyUnfinished() const
{
	return (_leftToRead || _transfer_chunked);
}

bool HttpHandler::isAllowedMethod(const std::string &method) const
{
	for (auto &allowed_method : _active_route->methods)
	{
		if (allowed_method == method)
			return true;
	}
	return false;
}

bool HttpHandler::findHeader(const std::string &header, std::string &value) const
{
	std::map<std::string, std::string>::const_iterator it = _request.map_headers.find(header);
	if (it != _request.map_headers.end())
	{
		value = it->second;
		return true;
	}
	return false;
}

bool HttpHandler::invalidRequest() const
{
	return (_request.method.empty() || _request.url.empty() || _request.version.empty());
}

// --------------------------------- SETTERS --------------------------------- //

//void HttpHandler::setReadyToWrite(bool ready)
//{
//	_ready_to_write = ready;
//}

// ---------------------------------- TIMER ---------------------------------- //

//void HttpHandler::startTimer()
//{
//	_timer.start();
//}

//void HttpHandler::stopTimer()
//{
//	_timer.stop();
//}

//bool HttpHandler::hasTimeOut()
//{
//	return _timer.hasTimeOut();
//}

// --------------------------------- METHODS --------------------------------- //

void HttpHandler::createStatusResponse(int code) {
	_response.status_code = std::to_string(code);
	try {
		_response.statusPhrase = _SUCCESS_STATUS.at(code);
	}
	catch (std::out_of_range) {
		std::cout << "Status code not found\n";
	}
}

void HttpHandler::resetRequestContext()
{
	_readStream.str(std::string());
	_readStream.seekp(0, std::ios_base::beg);
	_readStream.clear();
	_request_body_stream.str(std::string());
	_request_body_stream.seekp(0, std::ios_base::beg);
	_request_body_stream.clear();
	_request.method = "";
	_request.url = "";
	_request.version = "";
	_response_body_stream.str(std::string());
	_response_body_stream.clear();
	_response_header_stream.str(std::string());
	_response_header_stream.clear();
	//bzero(_overlapBuffer, OVERLAP);
	_active_route = &_default_route;
}

//void HttpHandler::saveOverlap(char *buffer, ssize_t nbytes)
//{
//	if (nbytes >= OVERLAP)
//	{
//		if (_overlapBuffer[0])
//			std::memcpy(buffer, _overlapBuffer, OVERLAP);
//		else
//			std::memcpy(buffer, buffer + OVERLAP, OVERLAP);
//		std::memcpy(_overlapBuffer, buffer + nbytes, OVERLAP); // save last 4 char
//	}
//	else if (nbytes)
//	{
//		std::memcpy(buffer, _overlapBuffer, OVERLAP);
//		std::memmove(_overlapBuffer, _overlapBuffer + nbytes, OVERLAP - nbytes); // moves left by nbytes
//		std::memcpy(_overlapBuffer + OVERLAP - nbytes, buffer + OVERLAP, nbytes);	// save last nbytes char
//	}
//}

void HttpHandler::writeToStream(char *buffer, ssize_t nbytes)
{
	_readStream.write(buffer, nbytes);
	if (_readStream.fail())
		throw std::runtime_error("writing to read stream");
}

int HttpHandler::writeToBody(char *buffer, ssize_t nbytes)
{
	if (!_leftToRead && !_transfer_chunked)
		return 0;
	if (_server->max_body_size && static_cast<ssize_t>(_request_body_stream.tellp()) + nbytes > _server->max_body_size)
	{
		_leftToRead = 0;
		_body_size_exceeded = true;
		return 0;
	}
	_request_body_stream.write(buffer, nbytes);
	if (_request_body_stream.fail())
		throw std::runtime_error("writing to request body stream");
	if (_leftToRead) // not chunked
	{
		_leftToRead -= nbytes;
		return _leftToRead > 0;
	}
	else if (_transfer_chunked) // chunked
		return _request_body_stream.str().find(EOF_CHUNKED) == std::string::npos;
	return 0;
}

void HttpHandler::parseRequest()
{
	// Parse the start-line
	_readStream >> _request.method >> _request.url >> _request.version;
	// Parse the headers into a hash table
	std::string header_name, header_value;
	while (getline(_readStream, header_name, ':') && getline(_readStream, header_value, '\r'))
	{
		// Remove any leading or trailing whitespace from the header value
		header_value.erase(0, header_value.find_first_not_of(" \r\n\t"));
		header_value.erase(header_value.find_last_not_of(" \r\n\t") + 1, header_value.length());
		header_name.erase(0, header_name.find_first_not_of(" \r\n\t"));
		_request.map_headers[header_name] = header_value;
	}
	_request.bodyLength = 0;
	std::string content_length_header;
	if (findHeader("Content-Length", content_length_header))
	{
		std::stringstream ss(content_length_header);
		ss >> _request.bodyLength;
	}
	std::string connection_header;
	_keepAlive = true;
	_transfer_chunked = false;
	if (findHeader("Connection", connection_header))
		_keepAlive = connection_header == "keep-alive";
	std::string transfer_encoding_header;
	if (findHeader("Transfer-Encoding", transfer_encoding_header))
		_transfer_chunked = transfer_encoding_header == "chunked";
	std::string host_header;
	if (findHeader("Host", host_header))
		_request.host = host_header.substr(0, host_header.find(":"));
	_leftToRead = _request.bodyLength;
	assignServerConfig();
}

void HttpHandler::setupRoute(const std::string &url)
{
	std::map<std::string, routes>::iterator it = _server->routes_map.begin();
	for (; it != _server->routes_map.end(); ++it)
	{
		if ((url.find(it->first) == 0 && it->first != "/") || (url == "/" && it->first == "/"))
		{
			_active_route = &it->second;
			if (_active_route->handler.empty())
				_request.url = _request.url.replace(url.find(it->first), it->first.length(), it->second.root);
			else
				_request.url = _active_route->handler;
			return;
		}
	}
	_request.url = _active_route->root + _request.url;
}

void HttpHandler::unchunckMessage()
{
	std::string line;

	while (std::getline(_request_body_stream, line))
	{
		int chunk_size = std::atoi(line.c_str());
		if (chunk_size == 0)
			break;
		std::string chunk;
		chunk.resize(chunk_size);
		_request_body_stream.read(&chunk[0], chunk_size);
		_request_body_stream.ignore(2);
		_response_body_stream.write(chunk.data(), chunk_size);
	}
	_request_body_stream.str("");
	_request_body_stream.clear();
	_request_body_stream << _response_body_stream.str();
	_response_body_stream.str("");
	_response_body_stream.clear();
	_request.map_headers.clear();
}

void HttpHandler::handleCGI(const std::string &original_url)
{
	std::string extension;
	size_t dotPos = _request.url.find_last_of('.');
	if (dotPos != std::string::npos)
		extension = _request.url.substr(dotPos);
	std::string cookies = "";
	_response.map_headers["Cookie"] = "";
	int err = CGIExecutor::run(_request, &_response_body_stream, &cookies, _active_route->handler, _active_route->cgi[extension], original_url);
	if (err)
		error(err);
	else
	{
		if (!cookies.empty())
			_response.map_headers["Set-Cookie"] = cookies;
		_response.status_code = "200";
		_response.statusPhrase = "OK";
		_response.map_headers["Content-Type"] = "text/html";
		_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
	}
}

void HttpHandler::redirection()
{
	createStatusResponse(301);
	_response.map_headers["Location"] = _active_route->redir;
	_response_body_stream << "<html><body><h1>301 Moved Permanently</h1></body></html>";
	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
}

void HttpHandler::assignServerConfig()
{
	if (_serverMap->empty())
		throw std::runtime_error("empty map");
	for (auto &[name, server] : *_serverMap) {
		if (server.is_default || server.host == "")
			_server = &server;
		if (server.host == _request.host) {
			_server = &server;
			return;
		}
	}
}

void HttpHandler::createHttpResponse()
{
	int index;
	std::string type[4] = {"GET", "POST", "DELETE", ""};
	_response.version = _request.version;
	std::string original_url = _request.url;

	if (!_server)
		throw std::runtime_error("No server found");
	setupRoute(_request.url);
	if (_transfer_chunked)
		unchunckMessage();
	if (invalidRequest())
		error(400);
	else if (_body_size_exceeded)
	{
		_body_size_exceeded = false;
		error(413);
	}
	else if (!_active_route->handler.empty())
		handleCGI(original_url);
	else if (!_active_route->redir.empty())
		redirection();
	else
	{
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

void HttpHandler::error(int error)
{
	resetRequestContext();
	std::string error_page = "";
	if (_server->error_pages.count(Utils::intToString(error)))
		error_page = _server->error_pages[Utils::intToString(error)];
	ErrorHandler error_handler = ErrorHandler(_response, _response_body_stream, error_page);
	error_handler.errorProcess(error);
}

void HttpHandler::generateDirectoryListing(const std::string &directory_path)
{
	DIR *dir = opendir(directory_path.c_str());
	if (dir == NULL)
		return error(403);
	_response_body_stream << "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><table>";
	_response_body_stream << "<tr><td><a href=\"../\">../</a></td><td>-</td></tr>"; // Link to parent directory
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		std::string path = directory_path + "/" + name;
		// Get the file size (if applicable)
		std::string size_str;
		if (entry->d_type == DT_REG)
		{
			struct stat st;
			if (stat(path.c_str(), &st) == 0)
			{
				std::stringstream ss;
				ss << st.st_size;
				size_str = ss.str() + " bytes";
			}
		}
		else
			size_str = "-";
		// Format the directory entry as an HTML table row with a link to the file or subdirectory
		std::string row;
		if (entry->d_type != DT_DIR)
		{
			// Link to a file
			std::string file_uri = _request.url.substr(_default_route.root.length()) + "/" + name;
			row = "<tr><td><a href=\"" + file_uri + "\">" + name + "</a></td><td>" + size_str + "</td></tr>";
		}
		_response_body_stream << row;
	}
	_response_body_stream << "</table></body></html>";
	closedir(dir);
}

void HttpHandler::GET()
{
	createStatusResponse(200);

	if (Utils::isDirectory(_request.url))
	{ // directory
		if (_active_route->autoindex)
			generateDirectoryListing(_request.url);
		else
		{
			if (_active_route->index == "")
				return error(404); // no index file
			_request.url += "/" + _active_route->index;
			if (Utils::pathToFileExist(_request.url))
				Utils::loadFile(_request.url, _response_body_stream);
			else
				return error(404);
		}
	}
	else if (Utils::pathToFileExist(_request.url))
		Utils::loadFile(_request.url, _response_body_stream);
	else
		return error(404);
	std::string content_type = getContentType(_request.url);
	if (content_type.empty())
		return error(415);
	_response.map_headers["Content-Type"] = content_type;
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
}

void HttpHandler::uploadFile(const std::string &contentType, size_t pos_boundary)
{
	std::string messageBody = _request_body_stream.str();
	const size_t headerPrefixLength = strlen("filename=\"");
	size_t pos1 = messageBody.find("filename=\"");
	size_t pos2 = pos1 != std::string::npos ? messageBody.find("\"", pos1 + headerPrefixLength) : std::string::npos;
	std::string fileName = pos2 != std::string::npos ? messageBody.substr(pos1 + headerPrefixLength, pos2 - pos1 - headerPrefixLength) : "";
	if (pos1 == std::string::npos || pos2 == std::string::npos || fileName.empty())
		return error(400);
	size_t start = messageBody.find(CRLF);
	size_t end = messageBody.find("\r\n--" + contentType.substr(pos_boundary + 9) + "--", start);
	if (start == std::string::npos || end == std::string::npos)
		return error(400);
	start += std::strlen(CRLF);
	messageBody = messageBody.substr(start, end - start);
	std::string path = _active_route->root + "/" + fileName;
	std::ofstream *outfile = Utils::createOrEraseFile(path);
	if (!outfile)
		return error(403);
	outfile->write(messageBody.c_str(), messageBody.length());
	outfile->close();
	delete outfile;
	createStatusResponse(201);
	_response_body_stream << messageBody;
	std::string content_type = getContentType(fileName);
	if (content_type.empty())
		return error(415);
	_response.map_headers["Content-Type"] = content_type;
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
	_response.map_headers["Location"] = path;
}

void HttpHandler::POST()
{
	createStatusResponse(200);
	std::string request_content_type;
	findHeader("Content-Type", request_content_type);
	size_t pos_boundary = request_content_type.find("boundary=");
	if (pos_boundary != std::string::npos) // multipart/form-data
		uploadFile(request_content_type, pos_boundary);
	else if (_request.url == "www/sendback")
		_response_body_stream << _request_body_stream.str();
	else if (request_content_type.find("application/x-www-form-urlencoded") != std::string::npos)
		_response_body_stream << "Response to application/x-www-form-urlencoded";
	else
		return error(501);
	_response.map_headers["Content-Length"] = Utils::intToString(_response_body_stream.str().length());
}

void HttpHandler::DELETE()
{
	createStatusResponse(204);

	std::string decoded_file_path = Utils::urlDecode(_request.url);
	if (!Utils::pathToFileExist(decoded_file_path))
		return error(404);
	if (std::remove(decoded_file_path.c_str()) != 0)
		return error(403);
}

void HttpHandler::constructStringResponse()
{
	bool first = true;
	_response.map_headers["Access-Control-Allow-Origin"] = "*";
	_response_header_stream << _response.version << " " << _response.status_code << " " << _response.statusPhrase << "\r\n";
	for (auto &[header, value] : _response.map_headers)
	{
		if (!first)
			_response_header_stream << "\r\n";
		_response_header_stream << header << ": " << value;
		first = false;
	}
	_response_header_stream << "\r\n\r\n";
	_response.map_headers.clear();
	_request.map_headers.clear();
}

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

const std::map<std::string, void (HttpHandler::*)()> HttpHandler::_HTTP_METHOD = {
	{"GET", static_cast<void (HttpHandler::*)()>(&HttpHandler::GET)},
	{"POST", static_cast<void (HttpHandler::*)()>(&HttpHandler::POST)},
	{"DELETE", static_cast<void (HttpHandler::*)()>(&HttpHandler::DELETE)}
};

const std::map<int, std::string> HttpHandler::_SUCCESS_STATUS = {
	{101, "Switching Protocols"},
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

HttpHandler::HttpHandler(int timeoutSeconds, server_name_level3 *serv_map) :
	_readStream(), _request_body_stream(), _response_header_stream(), _response_body_stream(),
 	_leftToRead(0), _serverMap(serv_map), _server(NULL),
 	_keepAlive(false), _bodySizeExceeded(false), _transferChunked(false), _isWebSocket(false),
	_default_route(), _active_route(&_default_route)
{
	_default_route.autoindex = false;
	_default_route.methods[0] = "GET";
	_default_route.methods[1] = "POST";
	_default_route.root = ROOT_PATH;
}

HttpHandler::~HttpHandler() {

}

// --------------------------------- GETTERS --------------------------------- //

std::string HttpHandler::getResponseHeader() const { return _response_header_stream.str(); }
std::string HttpHandler::getResponseBody() const { return _response_body_stream.str(); }
bool HttpHandler::isKeepAlive() const { return _keepAlive; }

std::string HttpHandler::getContentType(const std::string &path) const {
	std::string::size_type dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos) // no extension, assume directory
		return _active_route->autoindex ?  "text/html" : "text/plain";
	std::map<std::string, std::string>::const_iterator it = _MIME_TYPES.find(path.substr(dot_pos + 1));
	return it == _MIME_TYPES.end() ? "" : it->second;
}

bool HttpHandler::isBodyUnfinished() const {
	return (_leftToRead || _transferChunked);
}

bool HttpHandler::isAllowedMethod(const std::string &method) const {
	for (auto &allowed_method : _active_route->methods)
	{
		if (allowed_method == method)
			return true;
	}
	return false;
}

std::string HttpHandler::getHeaderValue(const std::string &header) const {
	std::map<std::string, std::string>::const_iterator it = _request.map_headers.find(header);
	if (it != _request.map_headers.end()) {
		return it->second;
	}
	return "";
}

bool HttpHandler::invalidRequestLine() const {
	return (_request.method.empty() || _request.url.empty() || _request.version.empty());
}

// --------------------------------- METHODS --------------------------------- //

void HttpHandler::createStatusResponse(int code) {
	_response.status_code = std::to_string(code);
	try {
		_response.statusPhrase = _SUCCESS_STATUS.at(code);
	}
	catch (std::out_of_range) {
		std::cout << "status code not found\n";
	}
}

void HttpHandler::resetRequestContext() {
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
	_active_route = &_default_route;
}

void HttpHandler::writeToStream(char *buffer, ssize_t nbytes) {
	_readStream.write(buffer, nbytes);
	if (_readStream.fail())
		throw std::runtime_error("writing to read stream");
}

int HttpHandler::writeToBody(char *buffer, ssize_t nbytes) {
	if (!_leftToRead && !_transferChunked)
		return 0;
	if (_server->max_body_size && static_cast<ssize_t>(_request_body_stream.tellp()) + nbytes > _server->max_body_size) {
		_leftToRead = 0;
		_bodySizeExceeded = true;
		return 0;
	}
	_request_body_stream.write(buffer, nbytes);
	if (_request_body_stream.fail())
		throw std::runtime_error("writing to request body stream");
	if (_leftToRead) { // not chunked
		_leftToRead -= nbytes;
		return _leftToRead > 0;
	}
	else if (_transferChunked) {
		bool unfinished = _request_body_stream.str().find(EOF_CHUNKED) == std::string::npos;
		if (!unfinished)
			unchunckMessage();
		return unfinished;
	}
	return 0;
}

void HttpHandler::parseRequest()
{

	std::cout << _readStream.str() << std::endl;

	_readStream >> _request.method >> _request.url >> _request.version;
	std::string header_name, header_value;
	while (getline(_readStream, header_name, ':') && getline(_readStream, header_value, '\r')) {
		header_value.erase(0, header_value.find_first_not_of(" \r\n\t"));
		header_value.erase(header_value.find_last_not_of(" \r\n\t") + 1, header_value.length());
		header_name.erase(0, header_name.find_first_not_of(" \r\n\t"));
		_request.map_headers[header_name] = header_value;
	}
	_request.bodyLength = 0;
	std::string content_length_header = getHeaderValue("Content-Length");
	if (!content_length_header.empty())
		_request.bodyLength = std::stoi(content_length_header);
	std::string connection = getHeaderValue("Connection");
	_keepAlive = connection == "keep-alive";
	_webSocketKey = getHeaderValue("Sec-WebSocket-Key");
	_isWebSocket = connection == "Upgrade" && getHeaderValue("Upgrade") == "websocket" && !_webSocketKey.empty();
	_transferChunked = getHeaderValue("Transfer-Encoding") == "chunked";
	_request.host = getHeaderValue("Host").substr(0, getHeaderValue("Host").find(":"));
	_leftToRead = _request.bodyLength;
	assignServerConfig();
}

void HttpHandler::setupRoute(const std::string &url)
{
	std::map<std::string, routes>::iterator it = _server->routes_map.begin();
	for (; it != _server->routes_map.end(); ++it) {
		if ((url.find(it->first) == 0 && it->first != "/") || (url == "/" && it->first == "/")) {
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
	while (std::getline(_request_body_stream, line)) {
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
	if (err) error(err);
	else {
		if (!cookies.empty())
			_response.map_headers["Set-Cookie"] = cookies;
		createStatusResponse(200);
		_response.map_headers["Content-Type"] = "text/html";
	}
}

void HttpHandler::redirection()
{
	createStatusResponse(301);
	_response.map_headers["Location"] = _active_route->redir;
	_response_body_stream << "<html><body><h1>301 Moved Permanently</h1></body></html>";
	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = std::to_string(_response_body_stream.str().length());
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

void HttpHandler::upgradeWebsocket() {
	createStatusResponse(101);

	std::string salt = _webSocketKey + GUID;
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(), sha1_hash);

    BIO *bio, *b64;
    BUF_MEM *bptr;

    bio = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_write(bio, sha1_hash, SHA_DIGEST_LENGTH);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bptr);

    std::string encoded(reinterpret_cast<char*>(bptr->data), bptr->length - 1);
    BIO_free_all(bio);

	_response.map_headers["Connection"] = "Upgrade";
	_response.map_headers["Upgrade"] = "websocket";
	_response.map_headers["Sec-WebSocket-Accept"] = encoded;
}

void HttpHandler::createHttpResponse()
{
	_response.version = _request.version;
	std::string original_url = _request.url;

	if (!_server)
		throw std::runtime_error("No server found");
	setupRoute(_request.url);
	if (invalidRequestLine()) error(400);
	else if (_bodySizeExceeded) {
		_bodySizeExceeded = false;
		error(413);
	}
	else if (!_active_route->handler.empty()) handleCGI(original_url);
	else if (!_active_route->redir.empty()) redirection();
	else if (_isWebSocket) upgradeWebsocket();
	else {
		auto it = HttpHandler::_HTTP_METHOD.find(_request.method);
		if (it != HttpHandler::_HTTP_METHOD.end() && isAllowedMethod(_request.method))
			(this->*(it->second))();
		else
			error(405);
	}
	constructStringResponse();
}

void HttpHandler::error(int error)
{
	resetRequestContext();
	std::string error_page = "";
	if (_server->error_pages.count(std::to_string(error)))
		error_page = _server->error_pages[std::to_string(error)];
	ErrorHandler error_handler = ErrorHandler(_response, _response_body_stream, error_page);
	error_handler.errorProcess(error);
}

void HttpHandler::generateDirectoryListing(const std::string &directory_path) {
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

void HttpHandler::uploadFile(const std::string &contentType, size_t pos_boundary) {
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
	_response.map_headers["Location"] = path;
}

void HttpHandler::GET() {
	createStatusResponse(200);

	if (Utils::isDirectory(_request.url)) { // directory
		if (_active_route->autoindex)
			generateDirectoryListing(_request.url);
		else {
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
}

void HttpHandler::POST() {
	createStatusResponse(200);
	std::string request_content_type = getHeaderValue("Content-Type");
	size_t pos_boundary = request_content_type.find("boundary=");
	if (pos_boundary != std::string::npos) // multipart/form-data
		uploadFile(request_content_type, pos_boundary);
	else if (_request.url == "www/sendback")
		_response_body_stream << _request_body_stream.str();
	else if (request_content_type.find("application/x-www-form-urlencoded") != std::string::npos)
		_response_body_stream << "Response to application/x-www-form-urlencoded";
	else
		return error(501);
}

void HttpHandler::DELETE() {
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
	if (_response.status_code == "200" || _response.status_code == "201") {
		_response.map_headers["Content-Length"] = std::to_string(_response_body_stream.str().length());
	}
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

	std::cout << _response_header_stream.str() << std::endl;
}

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

HttpHandler::HttpHandler(server_name_level3 *serv_map) :
	_serverMap(serv_map), _server(NULL),
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

bool HttpHandler::IsKeepAlive() const { return _keepAlive; }

std::string HttpHandler::GetContentType(const std::string &path) const {
	std::string::size_type dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos) // no extension, assume directory
		return _active_route->autoindex ?  "text/html" : "text/plain";
	std::map<std::string, std::string>::const_iterator it = _MIME_TYPES.find(path.substr(dot_pos + 1));
	return it == _MIME_TYPES.end() ? "" : it->second;
}

bool HttpHandler::HasBodyExceeded() const {
	return _bodySizeExceeded;
}

bool HttpHandler::IsAllowedMethod(const std::string &method) const {
	for (auto &allowed_method : _active_route->methods)
		if (allowed_method == method) return true;
	return false;
}

std::string HttpHandler::GetHeaderValue(const std::string &header) const {
	std::map<std::string, std::string>::const_iterator it = _request.map_headers.find(header);
	if (it != _request.map_headers.end()) {
		return it->second;
	}
	return "";
}

bool HttpHandler::InvalidRequestLine() const {
	return (_request.method.empty() || _request.url.empty() || _request.version.empty());
}

// --------------------------------- METHODS --------------------------------- //

int HttpHandler::WriteToBody(char *_request_body_buffer, const uint64_t &hasBeenRead, char* buffer, const ssize_t &nbytes) {
	if (BodyExceeded(hasBeenRead, nbytes))
		return -1;
    std::memcpy(_request_body_buffer + hasBeenRead, buffer, nbytes);

    if (_transferChunked)
		return TransferChunked(_request_body_buffer, hasBeenRead); // chunked
    return nbytes;
}

void HttpHandler::CreateStatusResponse(int code) {
	_response.status_code = std::to_string(code);
	try {
		_response.statusPhrase = _SUCCESS_STATUS.at(code);
	}
	catch (std::out_of_range) {
		std::cout << "status code not found\n";
	}
}

size_t HttpHandler::GetPositionEndHeader(char *buffer) {
	return ((std::string)(buffer - 4)).find(CRLF);  // ameliorable
}

void HttpHandler::ResetRequestContext() {
	_request.method = "";
	_request.url = "";
	_request.version = "";
	_request_body = "";
	_response_body_stream.str(std::string());
	_response_body_stream.clear();
	_response_header_stream.str(std::string());
	_response_header_stream.clear();
	_active_route = &_default_route;
}

bool HttpHandler::BodyExceeded(const uint64_t &hasBeenRead, const ssize_t &nbytes) {
	if (_server->max_body_size && (hasBeenRead + nbytes > _server->max_body_size)) {
		_bodySizeExceeded = true;
		return true;
	}
	return false;
}

int HttpHandler::TransferChunked(char *buffer, const uint64_t &size)
{
    std::string str = std::string(buffer, size);
	bool finished = str.find(EOF_CHUNKED) != std::string::npos;
	if (finished) {
        //UnchunckMessage(bodyStream);
        return -1;
    }
	return 0;
}

int HttpHandler::ParseRequest(std::stringstream &headerStream)
{
	headerStream >> _request.method >> _request.url >> _request.version;

	std::cout << headerStream.str() << std::endl;

	std::string header_name, header_value;
	while (getline(headerStream, header_name, ':') && getline(headerStream, header_value, '\r')) {
		header_value.erase(0, header_value.find_first_not_of(" \r\n\t"));
		header_value.erase(header_value.find_last_not_of(" \r\n\t") + 1, header_value.length());
		header_name.erase(0, header_name.find_first_not_of(" \r\n\t"));
		_request.map_headers[header_name] = header_value;
	}
	_request.bodyLength = 0;
	std::string content_length_header = GetHeaderValue("Content-Length");
	if (!content_length_header.empty())
		_request.bodyLength = std::stoi(content_length_header);
	std::string connection = GetHeaderValue("Connection");
	_keepAlive = connection == "keep-alive" | connection == "Upgrade";
	_isWebSocket = connection == "Upgrade" && GetHeaderValue("Upgrade") == "websocket";
	_transferChunked = GetHeaderValue("Transfer-Encoding") == "chunked";
	if (_transferChunked)
		_request.bodyLength = 1;
	_request.host = GetHeaderValue("Host").substr(0, GetHeaderValue("Host").find(":"));
	AssignServerConfig();
	return _request.bodyLength;
}

void HttpHandler::SetupRoute(const std::string &url)
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

void HttpHandler::UnchunckMessage(std::stringstream &bodyStream)
{
	std::stringstream requestBodyStream(std::ios::in | std::ios::out);
	int chunk_size;
	while (bodyStream >> chunk_size) {
		if (chunk_size == 0)
			break;
		std::string chunk;
		chunk.resize(chunk_size);
		bodyStream.ignore(2);
		bodyStream.read(&chunk[0], chunk_size);
		bodyStream.ignore(2);
		requestBodyStream.write(chunk.data(), chunk_size);
	}
	_request_body = requestBodyStream.str();
}

void HttpHandler::HandleCGI(const std::string &original_url)
{
	std::string extension;
	size_t dotPos = _request.url.find_last_of('.');
	if (dotPos != std::string::npos)
		extension = _request.url.substr(dotPos);
	std::string cookies = "";
	_response.map_headers["Cookie"] = "";
	int err = CGIExecutor::run(_request, _response_body_stream, &cookies, _active_route->handler, _active_route->cgi[extension], original_url);
	if (err) Error(err);
	else {
		if (!cookies.empty())
			_response.map_headers["Set-Cookie"] = cookies;
		CreateStatusResponse(200);
		_response.map_headers["Content-Type"] = "text/html";
	}
}

void HttpHandler::Redirection()
{
	CreateStatusResponse(301);
	_response.map_headers["Location"] = _active_route->redir;
	_response_body_stream << "<html><body><h1>301 Moved Permanently</h1></body></html>";
	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = std::to_string(_response_body_stream.str().length());
}

void HttpHandler::AssignServerConfig()
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

void HttpHandler::Handshake(const std::string &webSocketKey) {

    BIO *bio, *b64;
    BUF_MEM *bptr;

	const std::string salt = webSocketKey + GUID;
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(), sha1_hash);
    bio = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_write(bio, sha1_hash, SHA_DIGEST_LENGTH);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bptr);
    const std::string encoded(reinterpret_cast<char*>(bptr->data), bptr->length - 1);
    BIO_free_all(bio);
	_response.map_headers["Connection"] = "Upgrade";
	_response.map_headers["Upgrade"] = "websocket";
	_response.map_headers["Sec-WebSocket-Accept"] = encoded;

	//should communicate max_body_size to websockethandler;
}

void HttpHandler::CreateHttpResponse(char * request_body, const uint64_t &size)
{
	if (!_transferChunked)
        _request_body = std::string(request_body, size);

	_response.version = _request.version;
	const std::string original_url = _request.url;
	if (!_server)
		throw std::runtime_error("No server found");
	SetupRoute(_request.url);
	if (InvalidRequestLine()) Error(400);
	else if (_bodySizeExceeded) Error(413);
	else if (!_active_route->handler.empty()) HandleCGI(original_url);
	else if (!_active_route->redir.empty()) Redirection();
	else if (_isWebSocket) {
		CreateStatusResponse(101);
		Handshake(GetHeaderValue("Sec-WebSocket-Key"));
	}
	else {
		auto it = HttpHandler::_HTTP_METHOD.find(_request.method);
		if (it != HttpHandler::_HTTP_METHOD.end() && IsAllowedMethod(_request.method))
			(this->*(it->second))();
		else
			Error(405);
	}
	ConstructStringResponse();
}

void HttpHandler::Error(int error)
{
	ResetRequestContext();
	std::string error_page = "";
	if (_server->error_pages.count(std::to_string(error)))
		error_page = _server->error_pages[std::to_string(error)];
	ErrorHandler error_handler = ErrorHandler(_response, _response_body_stream, error_page);
	error_handler.errorProcess(error);
}

void HttpHandler::GenerateDirectoryListing(const std::string &directory_path) {
	DIR *dir = opendir(directory_path.c_str());
	if (dir == NULL)
		return Error(403);
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

void HttpHandler::UploadFile(const std::string &contentType, size_t pos_boundary) {
	std::string messageBody = _request_body;
	const size_t headerPrefixLength = strlen("filename=\"");
	size_t pos1 = messageBody.find("filename=\"");
	size_t pos2 = pos1 != std::string::npos ? messageBody.find("\"", pos1 + headerPrefixLength) : std::string::npos;
	std::string fileName = pos2 != std::string::npos ? messageBody.substr(pos1 + headerPrefixLength, pos2 - pos1 - headerPrefixLength) : "";
	if (pos1 == std::string::npos || pos2 == std::string::npos || fileName.empty())
		return Error(400);
	size_t start = messageBody.find(CRLF);
	size_t end = messageBody.find("\r\n--" + contentType.substr(pos_boundary + 9) + "--", start);
	if (start == std::string::npos || end == std::string::npos)
		return Error(400);
	start += std::strlen(CRLF);
	messageBody = messageBody.substr(start, end - start);
	std::string path = _active_route->root + "/" + fileName;
	std::ofstream *outfile = Utils::createOrEraseFile(path);
	if (!outfile)
		return Error(403);
	outfile->write(messageBody.c_str(), messageBody.length());
	outfile->close();
	delete outfile;
	CreateStatusResponse(201);
	_response_body_stream << messageBody;
	const std::string content_type = GetContentType(fileName);
	if (content_type.empty())
		return Error(415);
	_response.map_headers["Content-Type"] = content_type;
	_response.map_headers["Location"] = path;
}

void HttpHandler::GET() {
	CreateStatusResponse(200);
	if (Utils::isDirectory(_request.url)) { // directory
		if (_active_route->autoindex)
			GenerateDirectoryListing(_request.url);
		else {
			if (_active_route->index == "")
				return Error(404); // no index file
			_request.url += "/" + _active_route->index;
			if (Utils::pathToFileExist(_request.url))
				Utils::loadFile(_request.url, _response_body_stream);
			else
				return Error(404);
		}
	}
	else if (Utils::pathToFileExist(_request.url))
		Utils::loadFile(_request.url, _response_body_stream);
	else
		return Error(404);
	const std::string content_type = GetContentType(_request.url);
	if (content_type.empty())
		return Error(415);
	_response.map_headers["Content-Type"] = content_type;
}

void HttpHandler::POST() {
	CreateStatusResponse(200);
	const std::string request_content_type = GetHeaderValue("Content-Type");
	const size_t pos_boundary = request_content_type.find("boundary=");
	if (pos_boundary != std::string::npos) // multipart/form-data
		UploadFile(request_content_type, pos_boundary);
	else if (_request.url == "www/sendback")
		_response_body_stream << _request_body;
	else if (request_content_type.find("application/x-www-form-urlencoded") != std::string::npos)
		_response_body_stream << "Response to application/x-www-form-urlencoded";
	else
		return Error(501);
}

void HttpHandler::DELETE() {
	CreateStatusResponse(204);
	const std::string decoded_file_path = Utils::urlDecode(_request.url);
	if (!Utils::pathToFileExist(decoded_file_path))
		return Error(404);
	if (std::remove(decoded_file_path.c_str()) != 0)
		return Error(403);
}

void HttpHandler::ConstructStringResponse()
{
	bool first = true;
	_response.map_headers["Access-Control-Allow-Origin"] = "*";
	if (_response.status_code == "200" || _response.status_code == "201") {
		_response.map_headers["Content-Length"] = std::to_string(_response_body_stream.str().length());
	}
	_response_header_stream << _response.version << " " << _response.status_code << " " << _response.statusPhrase << "\r\n";
	for (auto &[header, value] : _response.map_headers)
	{
		if (!first) _response_header_stream << "\r\n";
		_response_header_stream << header << ": " << value;
		first = false;
	}
	_response_header_stream << "\r\n\r\n";
	_response.map_headers.clear();
	_request.map_headers.clear();

	std::cout << _response_header_stream.str() << std::endl;
}

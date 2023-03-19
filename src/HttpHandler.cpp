#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _readStream(new std::stringstream()),  _close_connection_mode(false), _type(0)
, _left_to_read(0) {
	_lastFour[0] = '\0';
}

HttpHandler::~HttpHandler() {
	delete _readStream;
}

void	HttpHandler::copyLastFour(char *buffer, ssize_t nbytes) {
	if (_lastFour[0])
		std::memcpy(buffer, _lastFour, 4);
	else
		std::memcpy(buffer, buffer + 4, 4);
	std::memcpy(_lastFour, buffer + nbytes, 4);		// copies last 4 bytes
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

bool HttpHandler::getConnectionMode() const {
	return _close_connection_mode;
}

void HttpHandler::addFileToResponse(const std::string &fileName) {
	std::ifstream input_file(fileName.c_str());
	_response.body << input_file.rdbuf();
}


void HttpHandler::parseRequest(const std::string &http_message) {

	// Parse the start-line
	std::stringstream stream(http_message);
	stream >> _request.method >> _request.path >> _request.version;

	// Parse the headers into a hash table
	std::map<std::string, std::string> map_headers;
	std::string header_name, header_value;
	while (getline(stream, header_name, ':') && getline(stream, header_value, '\r')) {
		// Remove any leading or trailing whitespace from the header value
		header_value.erase(0, header_value.find_first_not_of(" \r\n\t"));
		header_value.erase(header_value.find_last_not_of(" \r\n\t") + 1, header_value.length());
		header_name.erase(0, header_name.find_first_not_of(" \r\n\t"));
		map_headers[header_name] = header_value;
	}

	// Check if a message body is expected
	_request.body_length = 0;
	std::map<std::string, std::string>::iterator content_length_header = map_headers.find("Content-Length");
	if (content_length_header != map_headers.end()) {
		// Parse the content length header to determine the message body length
		std::stringstream ss(content_length_header->second);
		ss >> _request.body_length;
	}

	// Print out the parsed data
	std::cout << "Method: " << _request.method << std::endl;
	std::cout << "Path: " << _request.path << std::endl;
	std::cout << "Version: " << _request.version << std::endl;
	std::cout << "Headers: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = map_headers.begin(); it != map_headers.end(); ++it) {
		std::cout << it->first << ": " << it->second << std::endl;
	}

	_left_to_read = _request.body_length;
}

std::string HttpHandler::getContentType(const std::string& path) {
	return "empty";
}

bool HttpHandler::pathExists(const std::string& path) {
	return true;
}


std::string HttpHandler::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void HttpHandler::createResponse()
{
    // Set the HTTP version to the same as the request
    _response.version = _request.version;

    // Set a default response status code and phrase
    _response.status_code = 200;
    _response.status_phrase = "OK";

    // Check if the requested path exists and is accessible
    if (!pathExists(_request.path)) {
        _response.status_code = 404;
        _response.status_phrase = "Not Found";
        _response.body << "<html><body><h1>404 Not Found</h1></body></html>";
    }
    else {
        // Set the response body based on the requested path
        std::string content_type = getContentType(_request.path);
        addFileToResponse(_request.path);
        //_response.body << file_contents;

        // Add headers to the response
        _response.headers["Content-Type"] = content_type;
        _response.headers["Content-Length"] = intToString(_response.body.str().length());
    }

}

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
	_client.body_stream.write(buffer, nbytes);
	_left_to_read -= nbytes;
	return _left_to_read;
}

bool HttpHandler::getConnectionMode() const {
	return _close_connection_mode;
}

void HttpHandler::addFileToResponse(const std::string &fileName) {
	std::ifstream input_file(fileName.c_str());
	std::stringstream buffer;
	buffer << input_file.rdbuf();
	//_server._response += buffer.str();
}


void HttpHandler::parseRequest(const std::string &http_message) {

	// Parse the start-line
	std::stringstream stream(http_message);
	stream >> _client.method >> _client.path >> _client.version;

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
	_client.body_length = 0;
	std::map<std::string, std::string>::iterator content_length_header = map_headers.find("Content-Length");
	if (content_length_header != map_headers.end()) {
		// Parse the content length header to determine the message body length
		std::stringstream ss(content_length_header->second);
		ss >> _client.body_length;
	}

	// Print out the parsed data
	std::cout << "Method: " << _client.method << std::endl;
	std::cout << "Path: " << _client.path << std::endl;
	std::cout << "Version: " << _client.version << std::endl;
	std::cout << "Headers: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = map_headers.begin(); it != map_headers.end(); ++it) {
		std::cout << it->first << ": " << it->second << std::endl;
	}

	_left_to_read = _client.body_length;
}

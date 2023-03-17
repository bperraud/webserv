#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _readStream(new std::stringstream()),  _close_connection_mode(false), _type(0)
, _left_to_read(0) {

}

HttpHandler::~HttpHandler() {
	delete _readStream;
}

//std::string HttpHandler::addToRequest(const std::string &str) {
//	_client._request += str;
//	return _client._request;
//}

//std::string HttpHandler::addToResponse(const std::string &str) {
//	_server._responseServer += str;
//	return _server._response;
//}

//bool HttpHandler::hasBeenSend() const {
//	return _sended;
//}

void HttpHandler::writeToStream(char *buffer, ssize_t nbytes) {
	_readStream->write(buffer, nbytes);
}

void HttpHandler::writeToBody(char *buffer, ssize_t nbytes) {
	_client.body_stream.write(buffer, nbytes);
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


int HttpHandler::parseRequest(const std::string &http_message) {

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
	std::string message_body;

	std::map<std::string, std::string>::iterator content_length_header = map_headers.find("Content-Length");
	if (content_length_header != map_headers.end()) {
		// Parse the content length header to determine the message body length
		std::stringstream ss(content_length_header->second);
		ss >> _client.body_length;
	}

	if (_client.body_length != 0) {
		// Read the message body as a stream
		if (_client.body_length > 0) {
			message_body.resize(_client.body_length);
			stream.read(&message_body[0], _client.body_length);
		} else {
			// If no message body length was specified, read until the end of the stream
			getline(stream, message_body, '\0');
		}
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
	return 0;
}

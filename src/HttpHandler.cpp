#include "HttpHandler.hpp"


HttpHandler::HttpHandler() : _readStream(new std::stringstream()),  _close_connection_mode(false), _type(0){

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
	std::map<std::string, std::string> headers;
	std::string header_name, header_value;
	while (getline(stream, header_name, ':') && getline(stream, header_value, '\r')) {
		// Remove any leading or trailing whitespace from the header value
		header_value.erase(0, header_value.find_first_not_of(" \r\n\t"));
		header_value.erase(header_value.find_last_not_of(" \r\n\t") + 1);
		headers[header_name] = header_value;
	}

	// Check if a message body is expected
	int message_body_length = -1;
	std::string message_body;

	std::map<std::string, std::string>::iterator content_length_header = headers.find("Content-Length");
	if (content_length_header != headers.end()) {
		// Parse the content length header to determine the message body length
		std::stringstream ss(content_length_header->second);
		ss >> message_body_length;
	}

	if (message_body_length != -1) {
		// Read the message body as a stream
		if (message_body_length > 0) {
			message_body.resize(message_body_length);
			stream.read(&message_body[0], message_body_length);
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
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::cout << it->first << ": " << it->second;
	}
	std::cout << std::endl;

	if (message_body_length != -1)
		std::cout << "Message body: " << message_body << std::endl;

	return 0;
}

#include "ErrorHandler.hpp"
#include "HttpHandler.hpp"
#include "Utils.hpp"

ErrorHandler::ErrorHandler(HttpResponse& response, std::stringstream& body_stream, const std::string &error_page) :
	_response(response), _body_stream(body_stream), _error_page(error_page) {
}

const std::map<int, std::string>ErrorHandler::ERROR_MAP = {
	{400, "Bad Request"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{405, "Method Not Allowed"},
	{408, "Request Timeout"},
	{413, "Payload Too Large"},
	{415, "Unsupported Media Type"},
	{500, "Bad Request"},
	{501, "Internal Server Error"},
	{504, "Not Implemented"},
	{505, "HTTP Version Not Supported"},
};

ServerError::ServerError(HttpResponse& response, std::stringstream& body_stream, const std::string &error_page)
	: ErrorHandler(response, body_stream, error_page) {
}

ClientError::ClientError(HttpResponse& response, std::stringstream& stream, const std::string &error_page)
	: ErrorHandler(response, stream, error_page) {
}

void ErrorHandler::errorProcess(int error_code) {
	_response.status_code = std::to_string(error_code);
	try {
		_response.status_phrase = ERROR_MAP.at(error_code);
	}
	catch (std::out_of_range) {
		std::cout << "Error code not found\n";
	}
	if (_error_page.empty()) {
		std::string html_body = "<html><body><h1>" + _response.status_phrase + "</h1></body></html>";
		_body_stream << html_body ;
	}
	_response.map_headers["Content-Type"] = "text/html";
	_response.map_headers["Content-Length"] = Utils::intToString(_body_stream.str().length());
}

// ----------------------------- ClientError -----------------------------

ClientError::~ClientError() {}
ServerError::~ServerError() {}

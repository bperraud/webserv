#include "ErrorHandler.hpp"
#include "HttpHandler.hpp"

ErrorHandler::ErrorHandler(HttpResponse& response, std::stringstream& body_stream) : _response(response), _body_stream(body_stream) {}

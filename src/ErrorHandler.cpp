#include "ErrorHandler.hpp"

ErrorHandler::ErrorHandler(HttpMessage& request, std::stringstream& stream) : _request(request), _stream(stream) {}

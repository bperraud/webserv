#include "ProtocolHandlerInterface.hpp"

std::string ProtocolHandlerInterface::getResponseHeader() const { return _response_header_stream.str(); }

std::string ProtocolHandlerInterface::getResponseBody() const { return _response_body_stream.str(); }

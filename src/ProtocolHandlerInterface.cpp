#include "ProtocolHandlerInterface.hpp"

std::string ProtocolHandlerInterface::GetResponseHeader() const { return _response_header_stream.str(); }

std::string ProtocolHandlerInterface::GetResponseBody() const { return _response_body_stream.str(); }

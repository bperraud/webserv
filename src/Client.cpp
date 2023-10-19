#include "Client.hpp"




int Client::writeToBody(char *buffer, ssize_t nbytes)
{
//	if (!_leftToRead && !_transfer_chunked)
//		return 0;
//	if (_server->max_body_size && static_cast<ssize_t>(_request_body_stream.tellp()) + nbytes > _server->max_body_size)
//	{
//		_leftToRead = 0;
//		_body_size_exceeded = true;
//		return 0;
//	}
//	_request_body_stream.write(buffer, nbytes);
//	if (_request_body_stream.fail())
//		throw std::runtime_error("writing to request body stream");
//	if (_leftToRead) // not chunked
//	{
//		_leftToRead -= nbytes;
//		return _leftToRead > 0;
//	}
//	else if (_transfer_chunked) // chunked
//		return _request_body_stream.str().find(EOF_CHUNKED) == std::string::npos;
	return 0;

}

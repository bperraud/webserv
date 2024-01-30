#include "Client.hpp"

// --------------------------------- GETTERS --------------------------------- //

std::string Client::GetResponseHeader() const { return _protocolHandler->GetResponseHeader(); }
std::string Client::GetResponseBody() const { return _protocolHandler->GetResponseBody(); }
bool Client::IsKeepAlive() const { return _protocolHandler->IsKeepAlive(); }
bool Client::IsReadyToWrite() const { return _readyToWrite; }

Client::Client(int timeoutSeconds, server_name_level3 *serv_map) : _requestHeaderStream(std::ios::in | std::ios::out),
	_requestBodyStream(std::ios::in | std::ios::out), _serv_map(serv_map), _timer(timeoutSeconds),
	_protocolHandler(nullptr), _readyToWrite(false), _hasBodyExceeded(false), _lenStream(0), _hasBeenRead(0),
	_overlapBuffer(), _leftToRead(0) {
	_overlapBuffer[0] = '\0';
}

bool Client::HasBodyExceeded() const
{
    if (_hasBodyExceeded)
        return true;
	if (_protocolHandler)
		return _protocolHandler->HasBodyExceeded();
	return false;
}

// --------------------------------- SETTERS --------------------------------- //

void Client::SetReadyToWrite(bool ready) { _readyToWrite = ready; }

// ---------------------------------- TIMER ---------------------------------- //

void Client::StartTimer() { _timer.start(); }
void Client::StopTimer() { _timer.stop(); }
bool Client::HasTimeOut() { return _timer.hasTimeOut(); }

void Client::ResetRequestContext()
{
	_lenStream = 0;
	_leftToRead = 0;
    _hasBeenRead = 0;
    _hasBodyExceeded = false;
	bzero(_overlapBuffer, OVERLAP);
	_protocolHandler->ResetRequestContext();
	_readyToWrite = false;
	_requestHeaderStream.str(std::string());
	_requestHeaderStream.seekp(0, std::ios_base::beg);
	_requestHeaderStream.clear();
	_requestBodyStream.str(std::string());
	_requestBodyStream.seekp(0, std::ios_base::beg);
	_requestBodyStream.clear();

    free(_request_body_buffer);
}

void Client::DetermineRequestType(char * header)
{
	const bool mask_bit = *(header + 1) >> 7;
	if (mask_bit == 1)
		_protocolHandler = new WebSocketHandler(header);
	else
		_protocolHandler = new HttpHandler(_serv_map);
}

void Client::WriteToHeader(char *buffer, const ssize_t &nbytes)
{
	_requestHeaderStream.write(buffer, nbytes);
	_lenStream += nbytes;
	if (_requestHeaderStream.fail())
    {
		//throw std::runtime_error("writing to read stream");
        ;
	}
}

int Client::WriteToBody(char *buffer, const ssize_t &nbytes)
{
    uint64_t bytesRead = _protocolHandler->WriteToBody(_request_body_buffer, _hasBeenRead, buffer, nbytes);
    _hasBeenRead += nbytes;
    if (bytesRead == -1)
        return 0;
    _leftToRead -= bytesRead;
    return _leftToRead > 0;
}

int Client::WriteToStream(char *buffer, const ssize_t &nbytes)
{
	if (_leftToRead)
		return WriteToBody(buffer, nbytes);
	const size_t pos_end_header = _protocolHandler->GetPositionEndHeader(buffer);
	if (pos_end_header == std::string::npos)
    {
		WriteToHeader(buffer, nbytes);
		return (1);
	}
    else
    {
        WriteToHeader(buffer, pos_end_header);
        _leftToRead = _protocolHandler->ParseRequest(_requestHeaderStream);
        _request_body_buffer = new char[_leftToRead];
        if (!_request_body_buffer)
            _hasBodyExceeded = true;
	    return WriteToBody(buffer + pos_end_header, nbytes - pos_end_header);
    }
    return 0;
}

int Client::TreatReceivedData(char *buffer, const ssize_t &nbytes)
{
	StartTimer();
	SaveOverlap(buffer - OVERLAP, nbytes);
	if (_lenStream == 0 && nbytes >= 2)
		DetermineRequestType(buffer);
	return (WriteToStream(buffer, nbytes));
}

void Client::SaveOverlap(char *buffer, const ssize_t &nbytes)
{
	if (nbytes >= OVERLAP)
	{
		if (_overlapBuffer[0])
			std::memcpy(buffer, _overlapBuffer, OVERLAP);
		else
			std::memcpy(buffer, buffer + OVERLAP, OVERLAP);
		std::memcpy(_overlapBuffer, buffer + nbytes, OVERLAP); // save last 4 char
	}
	else if (nbytes)
	{
		std::memcpy(buffer, _overlapBuffer, OVERLAP);
		std::memmove(_overlapBuffer, _overlapBuffer + nbytes, OVERLAP - nbytes); // moves left by nbytes
		std::memcpy(_overlapBuffer + OVERLAP - nbytes, buffer + OVERLAP, nbytes); // save last nbytes char
	}
}

void Client::CreateResponse()
{
    _protocolHandler->CreateHttpResponse(_request_body_buffer, _hasBeenRead);
}

Client::~Client()
{
	delete _protocolHandler;
}

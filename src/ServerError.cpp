#include "ServerError.hpp"


ServerError::ServerError(HttpMessage& request, std::stringstream& stream) : ErrorHandler(request, stream) {

}

ServerError::~ServerError() {

}

void ServerError::errorProcess(int error) {
	(void) error;
}

void ServerError::internalServerError() {

}

void ServerError::notImplemented() {

}

void ServerError::gatewayTimeout() {

}

void ServerError::HTTPVersion() {

}

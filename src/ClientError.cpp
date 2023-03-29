#include "ClientError.hpp"

ClientError::ClientError(HttpMessage& request, std::stringstream& stream) : ErrorHandler(request, stream) {

}

ClientError::~ClientError() {

}

void ClientError::errorProcess(int error) {
	(void) error;
}

void ClientError::badRequest() {

}

void ClientError::notFound() {

}

void ClientError::methodNotAllowed() {

}

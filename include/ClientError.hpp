#ifndef CLIENTERROR_HPP
#define CLIENTERROR_HPP

#include "ErrorHandler.hpp"
#include <map>

class ClientError : public ErrorHandler {

private:
	std::map<int, void(ClientError::*)()> _error_map;

public:
    ClientError(HttpResponse& response, std::stringstream& body_stream);

	void errorProcess(int error);
	void badRequest(); 			// 400
	void notFound(); 			// 404
	void forbidden(); 			// 403
	void methodNotAllowed(); 	// 405
	void timeout(); 			// 408
	void payloadTooLarge(); 	// 413
    void unsupportedMediaType();// 415
	~ClientError();
};

#endif

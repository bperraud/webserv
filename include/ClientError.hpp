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
	void notFound();
	void forbidden();
	void badRequest();
	void methodNotAllowed();
	void timeout();
    ~ClientError();
};

#endif

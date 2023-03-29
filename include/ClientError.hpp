#ifndef CLIENTERROR_HPP
#define CLIENTERROR_HPP

#include "ErrorHandler.hpp"

class ClientError : public ErrorHandler {

private:


public:
    ClientError(HttpMessage& request, std::stringstream& stream);

	void errorProcess(int error);
	void badRequest();
	void notFound();
	void methodNotAllowed();

    ~ClientError();
};

#endif

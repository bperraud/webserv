#ifndef CGIEXECUTOR_HPP
#define CGIEXECUTOR_HPP

#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>		// errno
#include <stdio.h>		// perror

class CGIExecutor {
private:
	char**		_env;
    //std::string m_cgi_path;

public:
    // Constructor takes the path to the CGI binary as a parameter
    CGIExecutor(char** env);

    // Execute the CGI script with the given environment variables and input data
    void execute(char* path, int input_fd, int output_fd) const ;
};


#endif

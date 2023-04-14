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
#include "fcntl.h" 		// open()

struct HttpMessage;

class CGIExecutor {
private:
	char**	_env;

	CGIExecutor(const CGIExecutor &other);
	CGIExecutor &operator=(const CGIExecutor &other) ;

public:
	CGIExecutor();

	static CGIExecutor& getCgiInstance(){
		static CGIExecutor s_cgi;
		return s_cgi;
	}

	static void run (const HttpMessage &request) {
		CGIExecutor::getCgiInstance()._run(request);
	}

	void setEnv(char **env);
    // Execute the CGI script with the given environment variables and input data
    void execute(char* path, int input_fd, int output_fd)  ;
	void _run(const HttpMessage &request);

	void readCgiOutput(char *path);
};


#endif

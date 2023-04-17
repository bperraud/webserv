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
	void setupEnv(const HttpMessage &request, const std::string &url);

public:
	CGIExecutor();

	static CGIExecutor& getCgiInstance(){
		static CGIExecutor s_cgi;
		return s_cgi;
	}

	static void run (const HttpMessage &request, const std::string& path, const std::string &interpreter, const std::string &url) {
		CGIExecutor::getCgiInstance()._run(request, path, interpreter, url);
	}

	void setEnv(char **env);
    // Execute the CGI script with the given environment variables and input data
    std::string execute(const std::string &path, const std::string &interpreter)  ;
	std::string _run(const HttpMessage &request, const std::string& path, const std::string &interpreter, const std::string &url);

};


#endif

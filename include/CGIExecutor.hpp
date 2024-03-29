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

struct minishell {
	int		reader;
	int		writer;
	bool	running;
};

class CGIExecutor {
private:
	char**		_env;
	bool		_cgi_running;
	minishell	_minishell;

	CGIExecutor(const CGIExecutor &other);
	CGIExecutor &operator=(const CGIExecutor &other) ;
	void setupEnv(const HttpMessage &request, const std::string &url);

public:
	CGIExecutor() = default;

	static CGIExecutor& getCgiInstance(){
		static CGIExecutor s_cgi;
		return s_cgi;
	}

	static int run (const HttpMessage &request, std::stringstream &response_stream, std::string *cookies, const std::string& path, const std::string &interpreter, const std::string &url) {
		return CGIExecutor::getCgiInstance()._run(request, response_stream, cookies, path, interpreter, url);
	}

	std::string decodeUrl(const std::string& input);
	void setEnv(char **env);
    // Execute the CGI script with the given environment variables and input data
	int run_minishell(const HttpMessage &request, std::stringstream &response_stream, std::string *cookies);
	int run_minishell_cmd(const std::string &input, std::stringstream &response_stream, std::string *cookies);
    int execute(std::stringstream &response_stream, std::string *cookies, const std::string &path, const std::string &interpreter, const HttpMessage &request);
	int _run(const HttpMessage &request, std::stringstream &response_stream, std::string *cookies, const std::string &path, const std::string &interpreter, const std::string &url);
	void parse_response(std::stringstream &response_stream, std::string *cookies, std::string output);
};


#endif

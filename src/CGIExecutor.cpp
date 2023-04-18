#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"

CGIExecutor::CGIExecutor()
{
}

void CGIExecutor::setupEnv(const HttpMessage &request, const std::string &url)
{
	std::string request_method = "REQUEST_METHOD=" + request.method;
	putenv(const_cast<char *>(request_method.c_str()));

	std::string query_string = url.substr(url.find("?") + 1);
	setenv("QUERY_STRING", query_string.c_str(), 1);


	if (request.body_length)
	{
		std::string content_length = "CONTENT_LENGTH=" + (request.body_length ? request.map_headers.at("Content-Length") : 0);
		putenv(const_cast<char *>(content_length.c_str()));
	}
}

void CGIExecutor::setEnv(char **envp)
{
	_env = envp;
}

int CGIExecutor::_run(const HttpMessage &request, std::stringstream *response_stream, const std::string &path, const std::string &interpreter, const std::string &url)
{
	setupEnv(request, url);
	std::string script_name = request.url.substr(0, request.url.find("?"));
	if (!Utils::hasExecutePermissions(script_name.c_str())) {
	    return 403;
	}
	return execute(response_stream, path, interpreter);
}

int CGIExecutor::execute(std::stringstream *response_stream, const std::string &path, const std::string &interpreter)
{
	std::string res;
	int pipe_fd[2];

	if (pipe(pipe_fd) == -1)
		return 500;
	pid_t pid = fork();
	if (pid == -1)
		return 500;
	if (pid == 0)
	{
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);

		char parameter[path.length() + 1];
		strcpy(parameter, path.c_str());

		char script[interpreter.length() + 1];
		strcpy(script, interpreter.c_str());

		char *argv[] = {script, parameter, NULL};
		execvp(script, argv);
		return 500;
	}
	else
	{
		close(pipe_fd[1]);

		char buffer[1024];
		ssize_t n;
		while ((n = read(pipe_fd[0], buffer, 1024)) > 0)
			res.append(buffer, n);
		close(pipe_fd[0]);
	}
	waitpid(-1, NULL, 0);

	*response_stream << res ;

	std::cout << "CGI result :" << std::endl << res << std::endl;
	return 0;
}

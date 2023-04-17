#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"

CGIExecutor::CGIExecutor()
{
}

void CGIExecutor::setupEnv(const HttpMessage &request, const std::string &url)
{
	std::string request_method = "REQUEST_METHOD=" + request.method;
	putenv(const_cast<char *>(request_method.c_str()));
	
	std::string query_string = "QUERY_STRING=" + url.substr(url.find("?") + 1);
	putenv(const_cast<char *>(query_string.c_str()));
	std::cout << getenv("QUERY_STRING") << std::endl;
	
	if (request.body_length)
	{
		std::string content_length = "CONTENT_LENGTH=" + request.map_headers.at("Content-Length");
		putenv(const_cast<char *>(content_length.c_str()));
	}
}

void CGIExecutor::setEnv(char **envp)
{
	_env = envp;
}

std::string CGIExecutor::_run(const HttpMessage &request, const std::string &path, const std::string &interpreter, const std::string &url)
{
	setupEnv(request, url);
	std::string script_name = request.url.substr(0, request.url.find("?"));
	// if (!Utils::hasExecutePermissions(script_name.c_str())) {
	//     return "Status: 403\r\n\r\n";
	// }
	return execute(path, interpreter);
}

std::string CGIExecutor::execute(const std::string &path, const std::string &interpreter)
{
	std::string res;
	int pipe_fd[2];

	if (pipe(pipe_fd) == -1)
		return "Status: 500\r\n\r\n";
	pid_t pid = fork();
	if (pid == -1)
		return "Status: 500\r\n\r\n";
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
		return "Status: 500\r\n\r\n";
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

	std::cout << "CGI result :" << std::endl << res << std::endl;
	return res;
}

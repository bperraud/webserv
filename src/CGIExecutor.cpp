#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"

CGIExecutor::CGIExecutor()
{
}

void CGIExecutor::setEnv(char **envp)
{
	_env = envp;
}

std::string CGIExecutor::_run(const HttpMessage &request, const std::string &path, const std::string &interpreter)
{

	std::string REQUEST_METHOD = "REQUEST_METHOD=" + request.method;
	std::string QUERY_STRING = "QUERY_STRING=" + request.url.substr(request.url.find("?") + 1);

	if (request.body_length)
	{
		std::string CONTENT_LENGTH = "CONTENT_LENGTH=" + request.map_headers.at("Content-Length");
		putenv(const_cast<char *>(CONTENT_LENGTH.c_str()));
	}
	std::string SCRIPT_NAME = request.url.substr(0, request.url.find("?"));
	std::string SCRIPT_FILENAME = ROOT_PATH + SCRIPT_NAME;
	

	// if (!Utils::hasExecutePermissions(SCRIPT_FILENAME.c_str())) {
	// 	return 403;
	// }

	putenv(const_cast<char *>(REQUEST_METHOD.c_str()));
	putenv(const_cast<char *>(QUERY_STRING.c_str()));

	return execute(path, interpreter);
}

std::string CGIExecutor::execute(const std::string &path, const std::string &interpreter)
{
	std::string res;
	int pipe_fd[2];
	pipe(pipe_fd);
	pid_t pid = fork();
	if (pid == 0)
	{
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
		
		char path2[path.length() + 1];
		strcpy(path2, path.c_str());

		char interpreter2[interpreter.length() + 1];
		strcpy(interpreter2, interpreter.c_str());
		char *argv[] = {interpreter2, path2, NULL};
		execvp(interpreter2, argv);
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

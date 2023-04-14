#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"

CGIExecutor::CGIExecutor() {

}

void CGIExecutor::setEnv(char **envp) {
	_env = envp;
}

void CGIExecutor::_run(const HttpMessage &request) {
	std::string REQUEST_METHOD = "REQUEST_METHOD=" + request.method;
	std::string QUERY_STRING = "QUERY_STRING=" + request.url.substr(request.url.find("?") + 1);
	if  (request.body_length) {
		std::string CONTENT_LENGTH = "CONTENT_LENGTH=" + request.map_headers.at("Content-Length");
		putenv(const_cast<char*>(CONTENT_LENGTH.c_str()));
	}
	std::string SCRIPT_NAME = request.url.substr(0, request.url.find("?"));
	std::string SCRIPT_FILENAME = ROOT_PATH + SCRIPT_NAME;

	if (!Utils::hasExecutePermissions(SCRIPT_FILENAME.c_str())) {
		;//TODO: 403
	}

	putenv(const_cast<char*>(REQUEST_METHOD.c_str()));
	putenv(const_cast<char*>(QUERY_STRING.c_str()));
	char path[SCRIPT_FILENAME.size() + 1];
	std::strcpy(path, SCRIPT_FILENAME.c_str());
	//execute(path, input_fd, client_fd);
}

void CGIExecutor::readCgiOutput(char *path) {
	// Open the CGI script for reading its output
    FILE* cgi_output = popen(path, "r");
    if (cgi_output)
    {
        // Read and write the output of the CGI script
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), cgi_output) != NULL)
        {
            std::cout << buffer;
        }
        // Close the CGI script
        pclose(cgi_output);
    }
    else
    {
        // Error handling for failed popen() call
        throw std::runtime_error("Failed to execute CGI script\n");
    }
}

void CGIExecutor::execute(char *path, int input_fd, int output_fd)  {
	(void)input_fd;
	(void)output_fd;

	char* argv[] = {path, NULL};
	int pid = fork();
	if (pid < 0)
		throw std::runtime_error("Failed to fork process: ");
	if (pid == 0) {
		// child process
		//dup2(input_fd, STDIN_FILENO);
		dup2(output_fd, STDOUT_FILENO);
		//close(input_fd);
		close(output_fd);
		if (execve(path, argv, _env) < 0)
			throw std::runtime_error("Failed to execute CGI: ");
	} else {
		// parent process
		int status;
		waitpid(pid, &status, 0);
	}
}



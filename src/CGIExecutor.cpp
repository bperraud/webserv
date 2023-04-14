#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"


CGIExecutor::CGIExecutor(char** env) : _env(env) {

}


void CGIExecutor::run(const HttpMessage &request, int client_fd) {

	(void)client_fd;

	std::string REQUEST_METHOD = "REQUEST_METHOD=" + request.method;
	std::string QUERY_STRING = "QUERY_STRING=" + request.url.substr(request.url.find("?") + 1);

	if  (request.body_length) {
		std::string CONTENT_LENGTH = "CONTENT_LENGTH=" + request.map_headers.at("Content-Length");
		putenv(const_cast<char*>(CONTENT_LENGTH.c_str()));
	}
	std::string SCRIPT_NAME = request.url.substr(0, request.url.find("?"));

	std::string SCRIPT_FILENAME = ROOT_PATH + SCRIPT_NAME;

	//execute()

	//PATH_INFO: The path of the requested file.
	//SCRIPT_FILENAME: The full path of the CGI program.

	//int input_fd = open( "test", O_CREAT | O_RDWR | O_TRUNC, 0644);
	//client_fd = open( "output" , O_CREAT | O_RDWR | O_TRUNC, 0644);

	if (!Utils::hasExecutePermissions(SCRIPT_FILENAME.c_str())) {
		;//TODO: 403
	}

	putenv(const_cast<char*>(REQUEST_METHOD.c_str()));
	putenv(const_cast<char*>(QUERY_STRING.c_str()));
	//putenv(const_cast<char*>(SCRIPT_NAME.c_str()));
	//putenv(const_cast<char*>(SCRIPT_FILENAME.c_str()));

	char path[SCRIPT_FILENAME.size() + 1];
	std::strcpy(path, SCRIPT_FILENAME.c_str());

	//execute(path, input_fd, client_fd);
	readCgiOutput(path);
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

void CGIExecutor::execute(char *path, int input_fd, int output_fd) const {
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



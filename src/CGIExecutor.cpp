#include "CGIExecutor.hpp"


CGIExecutor::CGIExecutor(char** env) : _env(env) {

}


void CGIExecutor::run(const HttpMessage &request, int client_fd) {
	std::string REQUEST_METHOD = "REQUEST_METHOD=" + request.method;
	std::string QUERY_STRING = "QUERY_STRING=" + request.path.substr(request.path.find("?") + 1);

	if  (request.body_length) {
		std::string CONTENT_LENGTH = "CONTENT_LENGTH=" + request.map_headers.at("Content-Length");
		putenv(const_cast<char*>(CONTENT_LENGTH.c_str()));
	}
	std::string SCRIPT_NAME = request.path.substr(0, request.path.find("?"));

	std::string SCRIPT_FILENAME = ROOT_PATH + SCRIPT_NAME;

	//execute()

	//PATH_INFO: The path of the requested file.
	//SCRIPT_FILENAME: The full path of the CGI program.

	int input_fd = open( "test", O_CREAT | O_RDWR | O_TRUNC, 0644);

	client_fd = open( "output" , O_CREAT | O_RDWR | O_TRUNC, 0644);



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
        std::cerr << "Failed to execute CGI script\n";
    }
}

void CGIExecutor::execute(char *path, int input_fd, int output_fd) const {
	char* argv[] = {path, NULL};
	int pid = fork();
	if (pid < 0) {
		std::cerr << "Failed to fork process: " << strerror(errno) << std::endl;
		return;
	}
	if (pid == 0) {
		// child process
		//dup2(input_fd, STDIN_FILENO);
		dup2(output_fd, STDOUT_FILENO);
		//close(input_fd);
		close(output_fd);
		if (execve(path, argv, _env) < 0) {
			std::cerr << "Failed to execute CGI: " << strerror(errno) << std::endl;
			exit(EXIT_FAILURE);
		}
	} else {
		// parent process
		int status;
		waitpid(pid, &status, 0);
	}
}



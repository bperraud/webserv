#include "CGIExecutor.hpp"


CGIExecutor::CGIExecutor(char** env) : _env(env) {

}

void CGIExecutor::execute(char *path, int input_fd, int output_fd) const {

	//std::string cgi_path = path + extension;
	char* argv[] = {path, NULL};
	int pid = fork();
	if (pid < 0) {
		std::cerr << "Failed to fork process: " << strerror(errno) << std::endl;
		return;
	}
	if (pid == 0) {
		// child process
		dup2(input_fd, STDIN_FILENO);
		dup2(output_fd, STDOUT_FILENO);
		close(input_fd);
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



#include "CGIExecutor.hpp"
#include "HttpHandler.hpp"

extern char **environ;

std::string CGIExecutor::decodeUrl(const std::string& input) {
    std::ostringstream decoded;
    std::istringstream encoded(input);

    char c;
    int hex;
    while (encoded.get(c)) {
        if (c == '%') {
            char hex1, hex2;
            if (encoded.get(hex1) && encoded.get(hex2)) {
                int digit1 = hex1 >= 'A' ? (hex1 & 0xDF) - 'A' + 10 : (hex1 - '0');
                int digit2 = hex2 >= 'A' ? (hex2 & 0xDF) - 'A' + 10 : (hex2 - '0');
                char decodedChar = (digit1 << 4) + digit2;
                decoded << decodedChar;
            } else {
                // Invalid encoding, handle error
            }
        }
		else if (c == '+')
            decoded << ' ';
        else
            decoded << c;
    }

    return decoded.str();
}

void CGIExecutor::setupEnv(const HttpRequest &request, const std::string &url)
{
	std::string request_method = "REQUEST_METHOD=" + request.method;
	putenv(const_cast<char *>(request_method.c_str()));
	std::string query_string = url.substr(url.find("?") + 1);
	std::string decode = decodeUrl(query_string);
	setenv("QUERY_STRING", decodeUrl(query_string).c_str(), 1);

	if (request.bodyLength)
	{
		std::string content_length = "CONTENT_LENGTH=" + (request.bodyLength ? request.map_headers.at("Content-Length") : 0);
		putenv(const_cast<char *>(content_length.c_str()));
	}
}

void CGIExecutor::setEnv(char **envp)
{
	_env = envp;
}

int CGIExecutor::_run(const HttpRequest &request, std::stringstream &response_stream, std::string *cookies, const std::string &path, const std::string &interpreter, const std::string &url)
{
	setupEnv(request, url);
	std::string script_name = request.url.substr(0, request.url.find("?"));

	if (script_name == "www/cgi/minishell") {
		std::string input = getenv("QUERY_STRING");
		if (_minishell.running)
			return run_minishell_cmd(input, response_stream, cookies);
		return run_minishell(request, response_stream, cookies);
	}
	if (!Utils::hasExecutePermissions(script_name.c_str()))
	    return 403;
	if (!Utils::pathToFileExist(script_name.c_str()))
	    return 404;
	return execute(response_stream, cookies, path, interpreter, request);
}

int CGIExecutor::run_minishell(const HttpRequest &request, std::stringstream &response_stream, std::string *cookies)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		return 1;
	} else if (pid == 0) {
		execl("www/cgi/minishell", "www/cgi/minishell", NULL);
		perror("execl");
		exit(1);
	}
	else {
		const char* writingPipe = "pipeA";
		const char* readingPipe = "pipeB";
		mkfifo(writingPipe, 0666);
		mkfifo(readingPipe, 0666);
		_minishell.writer = open(writingPipe, O_RDWR);
		_minishell.reader = open(readingPipe, O_RDWR);
		_minishell.running = true;
	}
	parse_response(response_stream, cookies, "success");
	return 0;
}

int CGIExecutor::run_minishell_cmd(const std::string &input, std::stringstream &response_stream, std::string *cookies) {
	char character;
	const char MESSAGE_DELIMITER = '\0';
	std::string res;

	if (write(_minishell.writer, input.c_str(), input.length() + 1) < 0)
		perror("write");

	if (input == "exit") {
		_minishell.running = false;
		close(_minishell.reader);
		close(_minishell.writer);
		return 0;
	}
	while (read(_minishell.reader, &character, sizeof(character))) {
		if (character == MESSAGE_DELIMITER)
			break;
		res += character;
	}
	parse_response(response_stream, cookies, res);
	return 0;
}


int CGIExecutor::execute(std::stringstream &response_stream, std::string *cookies, const std::string &path, const std::string &interpreter, const HttpRequest &request)
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
		if (request.map_headers.count("Cookie") > 0) {
			std::string cookie_values = request.map_headers.at("Cookie");
			setenv("HTTP_COOKIE", cookie_values.c_str(), 1);
		}
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);

		char parameter[path.length() + 1];
		strcpy(parameter, path.c_str());

		char script[interpreter.length() + 1];
		strcpy(script, interpreter.c_str());

		char *argv[] = {script, parameter, NULL};
		execve(script, argv, environ);
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
	parse_response(response_stream, cookies, res);
	return 0;
}

void CGIExecutor::parse_response(std::stringstream &response_stream, std::string *cookies, std::string output) {
	std::string::size_type pos = output.find("\r\n\r\n");
	std::istringstream headerStream(output.substr(0, pos));
	std::string headerLine;

	if (pos == std::string::npos) {
		response_stream << output;
		return ;
	}

	while (std::getline(headerStream, headerLine, '\n')) {
		std::string::size_type colonPos = headerLine.find(":");
		std::string headerName = headerLine.substr(0, colonPos);
        std::string headerValue = headerLine.substr(colonPos + 1);
		if (headerName == "Set-Cookie") {
			*cookies = headerValue;
		}
	}
	std::string body = output.substr(pos + 4);
	response_stream << body;
}

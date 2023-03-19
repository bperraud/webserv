#include "ServerManager.hpp"


ServerManager::ServerManager(Config config) {
	(void) config;
	_listen_fd = 0;
}

void	ServerManager::setupSocket() {
	if ((_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}
	memset((char *)&_host_addr, 0, sizeof(_host_addr));
	int _host_addrlen = sizeof(_host_addr);
	_host_addr.sin_family = AF_INET;  // AF_INET for IPv4 Internet protocols
	// htonl converts a long integer (e.g. address) to a network representation
	// htons converts a short integer (e.g. port) to a network representation
	_host_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any address = 0.0.0.0
	_host_addr.sin_port = htons(PORT);

	int enable_reuseaddr = 1;
	if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuseaddr, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
		exit(EXIT_FAILURE);
	}

	#if 1
	// disables the Nagle algorithm, which can improve performance for small messages,
	//but can degrade performance for large messages or bulk data transfer.
	int enable_nodelay = 1;
	if (setsockopt(_listen_fd, IPPROTO_TCP, TCP_NODELAY, &enable_nodelay, sizeof(int)) < 0) {
		perror("setsockopt(TCP_NODELAY) failed");
		exit(EXIT_FAILURE);
	}
	#endif

	if (bind(_listen_fd, (struct sockaddr *) &_host_addr, _host_addrlen) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	setNonBlockingMode(_listen_fd);
	// SOMAXCONN = maximum number of pending connections queued up before connections are refused
	if (listen(_listen_fd, SOMAXCONN) < 0)
	{
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	printf("server listening for connections...\n");
}

void ServerManager::setNonBlockingMode(int socket) {
	if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0) {
		perror("Failed to set socket to non-blocking mode");
		close(socket);
	}
}

void ServerManager::handleNewConnectionsEpoll() {
	// Create the epoll file descriptor
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd < 0) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}
	// Add the listen socket to the epoll interest list
	struct epoll_event event;
	event.data.fd = _listen_fd;
	event.events = EPOLLIN;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _listen_fd, &event) < 0) {
		perror("epoll_ctl EPOLL_CTL_ADD");
		exit(EXIT_FAILURE);
	}
	struct epoll_event events[MAX_EVENTS];
	// events array is used to store events that occur on any of
	// the file descriptors that have been registered with epoll_ctl()

	while (1) {
		std::cout << "waiting..." << std::endl;
		int n_ready = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
		// if any of the file descriptors match the interest then epoll_wait can return without blocking.
		if (n_ready == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			// If the listen socket is ready, accept a new connection and add it to the epoll interest list
			if (fd == _listen_fd) {
				struct sockaddr_in client_addr;
				socklen_t client_addrlen = sizeof(client_addr);
				int newsockfd = accept(_listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
				if (newsockfd < 0) {
					perror("accept()");
					exit(EXIT_FAILURE);
				}
				setNonBlockingMode(newsockfd);
				event.data.fd = newsockfd;
				event.events = EPOLLIN;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
					perror("epoll_ctl EPOLL_CTL_ADD");
					exit(EXIT_FAILURE);
				}
				_client_map.insert(std::make_pair(newsockfd, new HttpHandler()));
				std::cout << "new connection accepted for client on socket : " << newsockfd << std::endl;
			}
			else {
				if (!readFromClient(fd)) {	// if no error, read complete

					std::cout << "All : " << std::endl;
					std::cout << _client_map[fd]->getRequest() << std::endl;

					//std::cout << "Message body: " << std::endl;
					//std::cout << _client_map[fd]->getBody() << std::endl;
					//_client_map[fd].addFileToResponse("./website/index.html");
					writeToClient(fd);
					connectionCloseMode(fd);
					_client_map[fd]->bbzero();
				}
			}
		}
	}
}

void ServerManager::connectionCloseMode(int client_fd) {
	if (_client_map[client_fd]->getConnectionMode())
		closeClientConnection(client_fd);
}

void ServerManager::closeClientConnection(int client_fd) {
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
		perror("epoll_ctl EPOLL_CTL_DEL");
		exit(EXIT_FAILURE);
	}
	delete _client_map[client_fd];
	_client_map.erase(client_fd);
	close(client_fd);
}

// return 0 if read complete, 1 otherwise
int	ServerManager::readFromClient(int client_fd) {
	char buffer[BUFFER_SIZE + 4];
	ssize_t nbytes = recv(client_fd, buffer + 4, BUFFER_SIZE, 0);

	_client_map[client_fd]->rmemcpy(buffer, nbytes);

	if (nbytes == -1) {
		perror("recv()");
		closeClientConnection(client_fd);
		return 1;
	}
	else if (nbytes == 0) {
		closeClientConnection(client_fd);
		printf("connection closed on client %d\n", client_fd);
		return 1;
	}
	else {
		printf("finished reading data from client %d\n", client_fd);


		size_t pos_end_header = ((std::string)buffer).find("\r\n\r\n");

		if (pos_end_header == std::string::npos) {			// not found
			_client_map[client_fd]->writeToStream(buffer + 4, nbytes);
			return 1;
		}
		else {
			int pos = pos_end_header - 4;
			_client_map[client_fd]->writeToStream(buffer + 4, pos);
			//_client_map[client_fd]->parseRequest(_client_map[client_fd]->getRequest());
			//_client_map[client_fd]->writeToBody(buffer + 4 + pos + 4, nbytes - pos - 4);
			_client_map[client_fd]->subLeftToRead(nbytes - pos);
			return 0;
		}
	}
	return 1;
}

int ServerManager::writeToClient(int client_fd) {
	std::string response = "HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html\r\n"
					"Content-Length: 30\r\n"
					"Connection: close\r\n\r\n"
					"<html><body>Hello my world!</body></html>";

	ssize_t nbytes = send(client_fd, response.c_str(), response.length(), 0);
	if (nbytes == -1) {
		perror("send()");
		return 1;
	}
	else if ((size_t) nbytes == response.length()) {
		printf("finished writing %d\n", client_fd);
	}
	return 0;
}


bool ServerManager::isEOF(const std::string& str) {
    // Find the last occurrence of a newline character
    return (str.rfind("\r\n\r\n") != std::string::npos);
}

ServerManager::~ServerManager() {

}

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
	// Set socket to non-blocking mode
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags < 0) {
		perror("Failed to get socket flags");
		close(socket);
	}
	if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
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

	std::string response = "HTTP/1.1 200 OK\r\n"
					   "Content-Type: text/html\r\n"
					   "Content-Length: 30\r\n"
					   "Connection: close\r\n\r\n"
					   "<html><body>Hello my world!</body></html>";

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
				// Add the new socket to the epoll interest list
				event.data.fd = newsockfd;
				event.events = EPOLLIN; // ready to read from client
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
					perror("epoll_ctl EPOLL_CTL_ADD");
					exit(EXIT_FAILURE);
				}
				// add new Client
				_client_map.insert(std::make_pair(newsockfd, ClientRequest()));
				std::cout << "new connection accepted for client on socket : " << newsockfd << std::endl;
			}
			else {
				if (!readFromClient(fd)) {	// if no error, read done -> write

					std::cout << "end of read" << std::endl;
					std::cout << _client_map[fd].getRequest() << std::endl;
					writeToClient(fd, response);
					connectionCloseMode(fd);
				}
			}
		}
	}
}

void ServerManager::connectionCloseMode(int client_fd) {
	ClientRequest request = _client_map[client_fd];
	if (request.getConnectionMode())
		closeClientConnection(client_fd);
}

void ServerManager::closeClientConnection(int client_fd) {
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
		perror("epoll_ctl EPOLL_CTL_DEL");
		exit(EXIT_FAILURE);
	}
	_client_map.erase(client_fd);
	close(client_fd);
}

int	ServerManager::readFromClient(int client_fd) {
	char buffer[BUFFER_SIZE];
	ssize_t nbytes = recv(client_fd, buffer, BUFFER_SIZE, 0);
	if (nbytes == -1) {
		perror("recv()");
		return 1;
	}
	else if (nbytes == 0) {
		closeClientConnection(client_fd);
		printf("connection closed on client %d\n", client_fd);
		return 1;
	}
	else {
		printf("finished reading data from client %d\n", client_fd);
		std::string request = addToClientRequest(client_fd, std::string(buffer, nbytes));
		return (isEOF(request));
	}
	return 1;
}

std::string ServerManager::addToClientRequest(int client_fd, const std::string &str) {
	return _client_map[client_fd].addToRequest(str);
}

int ServerManager::writeToClient(int client_fd, const std::string& data) {
	ssize_t nbytes = send(client_fd, data.c_str(), data.length(), 0);
	if (nbytes == -1) {
		perror("send()");
		return 1;
	}
	else if ((size_t) nbytes == data.length()) {
		printf("finished writing %d\n", client_fd);
	}
	return 0;
}

bool ServerManager::isEOF(const std::string& buffer) {
    // Find the last occurrence of a newline character
    size_t last_newline_pos = buffer.rfind("\r\n\r\n");
    return (last_newline_pos == std::string::npos);
}


void ServerManager::handleRequests(int client_fd) {
	(void) client_fd;
}

ServerManager::~ServerManager() {

}

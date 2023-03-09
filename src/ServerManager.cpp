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
	int epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	// Add the listen socket to the epoll interest list
	struct epoll_event event;
	event.data.fd = _listen_fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _listen_fd, &event) < 0) {
		perror("epoll_ctl EPOLL_CTL_ADD");
		exit(EXIT_FAILURE);
	}
	// Main epoll event loop
	struct epoll_event events[MAX_EVENTS];
	// events array is used to store events that occur on any of
	// the file descriptors that have been registered with epoll_ctl()

	char resp[] = "HTTP/1.0 200 OK\r\n"
				"Server: webserver-epoll\r\n"
				"Content-type: text/html\r\n"
				"Connection: keep-alive\r\n\r\n"
				"<html>hello, epoll world</html>\r\n";


	while (1) {
		int n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		// if any of the file descriptors match the interest then epoll_wait can return without blocking.

		if (n_ready == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			std::cout << "fd number : " << fd << std::endl;
			// If the listen socket is ready, accept a new connection and add it to the epoll interest list
			if (fd == _listen_fd) {
				struct sockaddr_in client_addr;
				socklen_t client_addrlen = sizeof(client_addr);
				int newsockfd = accept(_listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
				if (newsockfd < 0) {
					perror("accept");
					continue;
				}
				setNonBlockingMode(newsockfd);
				// Add the new socket to the epoll interest list
				event.data.fd = newsockfd;
				event.events = EPOLLIN | EPOLLET; // ready to read from client
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
					perror("epoll_ctl EPOLL_CTL_ADD");
					exit(EXIT_FAILURE);
				}
				std::cout << "new connection accepted for client on socket : " << newsockfd << std::endl;
			}

			// Handle read event
			// read as much data as we can

			#if 1
			else {
				readFromClient(fd);
				writeToClient(fd, resp);

				// Remove the socket from the epoll interest list
				if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
					perror("epoll_ctl EPOLL_CTL_DEL");
					exit(EXIT_FAILURE);
				}
				close(fd);
			}

			#else
			else {
				ssize_t bytes_read = recv(fd, buffer, BUFFER_SIZE, 0);
				if (bytes_read == -1) {
					if (errno == EWOULDBLOCK || errno == EAGAIN) {
						continue;
					} else {
						perror("recv");
						exit(EXIT_FAILURE);
					}
				}
				else if (bytes_read == 0) {
					// connection closed
					std::cout << "no data" << std::endl;
					close(fd);
					continue;
				}
				buffer[bytes_read] = '\0';
				std::cout << buffer << std::endl;

				// Process the request
				//process_request(buffer, bytes_read);
				char resp[] = "HTTP/1.0 200 OK\r\n"
							"Server: webserver-epoll\r\n"
							"Content-type: text/html\r\n"
							"Connection: keep-alive\r\n\r\n"
							"<html>hello, epoll world</html>\r\n";

				int valwrite = send(fd, resp, strlen(resp), 0);
				if (valwrite < 0) {
					perror("send");
					continue;
				}
				std::cout << "request processed for client on socket : " << fd << std::endl;

				#if 1
				// Remove the socket from the epoll interest list
				if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
					perror("epoll_ctl EPOLL_CTL_DEL");
					exit(EXIT_FAILURE);
				}
				close(fd);
				#endif
			}

			#endif
		}
	}
}

int	ServerManager::readFromClient(int socket) {
	char buffer[BUFFER_SIZE];
	while (1) {
		ssize_t nbytes = recv(socket, buffer, BUFFER_SIZE, 0);
		if (nbytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("finished reading data from client\n");
				buffer[nbytes] = '\0';
				std::cout << buffer << std::endl;
				return 0;
			}
			else {
				perror("recv()");
				return 1;
			}
		}
		else if (nbytes == 0) {
			printf("finished recv with %d\n", socket);
			return 0;
		}
	}
}

int ServerManager::writeToClient(int socket, const char* data) {
	ssize_t nbytes_total = 0;
	while (nbytes_total < strlen(data)) {
		ssize_t nbytes = send(socket, data + nbytes_total, strlen(data) - nbytes_total, 0);
		if (nbytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("finished writing data to client\n");
				return 0;
			}
			else {
				perror("send()");
				return 1;
			}
		}
		else {
			nbytes_total += nbytes;
		}
	}
	printf("finished writing data to client\n");
	return 0;
}

void ServerManager::handleRequests(int client_fd) {

}

ServerManager::~ServerManager() {

}

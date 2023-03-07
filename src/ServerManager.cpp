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
	if (bind(_listen_fd, (struct sockaddr *) &_host_addr, _host_addrlen) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	// SOMAXCONN = maximum number of pending connections queued up before connections are refused
	if (listen(_listen_fd, SOMAXCONN) < 0)
	{
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	printf("server listening for connections...\n");
}


void	ServerManager::handleNewConnections() {
	char buffer[BUFFER_SIZE];

	char resp[] = "HTTP/1.0 200 OK\r\n"
                  "Server: webserver-c\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<html>hello, world</html>\r\n";

	// Create client address
    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

	while (1) {
		// Accept incoming connections
		int newsockfd = accept(_listen_fd, (struct sockaddr *)&_host_addr, (socklen_t *)&_host_addrlen);
		// host_addrlen is set to number of bytes of data actually stored by the kernel in socket address structure
		if (newsockfd < 0) {
			perror("webserver (accept)");
			continue;
		}
		printf("connection accepted on socket %i\n", newsockfd);

		// Read from the socket
        int valread = read(newsockfd, buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("webserver (read)");
            continue;
        }
		buffer[valread] = '\0';
		std::cout << buffer << std::endl;

		// Get client address
        int client_socket = getsockname(newsockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);
        if (client_socket < 0) {
            perror("webserver (getsockname)");
            continue;
        }
		printf("[%s:%u]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// Write to the socket
        int valwrite = write(newsockfd, resp, strlen(resp));
        if (valwrite < 0) {
            perror("webserver (write)");
            continue;
        }

		close(newsockfd);
	}
}


void ServerManager::handleNewConnectionsEpoll() {

	char buffer[BUFFER_SIZE];

	// Create the epoll file descriptor
	int epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	// Add the listen socket to the epoll interest list
	struct epoll_event event;
	event.data.fd = _listen_fd;
	event.events = EPOLLIN;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _listen_fd, &event) < 0) {
		perror("epoll_ctl EPOLL_CTL_ADD");
		exit(EXIT_FAILURE);
	}

	// Main epoll event loop
	struct epoll_event events[MAX_EVENTS];

	// events array is used to store events that occur on any of
	// the file descriptors that have been registered with epoll_ctl()
	while (1) {
		int n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (n_ready == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			std::cout << "fd number : " << i << std::endl;
			// If the listen socket is ready, accept a new connection and add it to the epoll interest list
			if (fd == _listen_fd) {
				struct sockaddr_in client_addr;
				socklen_t client_addrlen = sizeof(client_addr);
				int newsockfd = accept(_listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
				if (newsockfd == -1) {
					perror("accept");
					continue;
				}
				// Add the new socket to the epoll interest list
				event.data.fd = newsockfd;
				event.events = EPOLLIN;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
					perror("epoll_ctl EPOLL_CTL_ADD");
					exit(EXIT_FAILURE);
				}
				std::cout << "new connection accepted for client on socket : " << newsockfd << std::endl;
			}

			// If a socket is ready for reading, read the request and send the response
			else {

				int valread = read(fd, buffer, BUFFER_SIZE);
				if (valread < 0) {
					perror("read");
					continue;
				}

				buffer[valread] = '\0';
				std::cout << buffer << std::endl;

				char resp[] = "HTTP/1.0 200 OK\r\n"
							"Server: webserver-epoll\r\n"
							"Content-type: text/html\r\n"
							"Connection: keep-alive\r\n\r\n"
							"<html>hello, epoll world</html>\r\n";

				int valwrite = write(fd, resp, strlen(resp));
				if (valwrite < 0) {
					perror("write");
					continue;
				}

				std::cout << "request processed for client on socket : " << fd << std::endl;

				#if 0
				// Remove the socket from the epoll interest list
				if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
					perror("epoll_ctl EPOLL_CTL_DEL");
					exit(EXIT_FAILURE);
				}
				#endif

				close(fd);
			}
		}
	}
}


void ServerManager::handleRequests(int client_fd) {

}

ServerManager::~ServerManager() {

}

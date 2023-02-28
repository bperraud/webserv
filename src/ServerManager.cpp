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

	struct sockaddr_in host_addr;
	memset((char *)&host_addr, 0, sizeof(host_addr));
	int host_addrlen = sizeof(host_addr);
	host_addr.sin_family = AF_INET;  // AF_INET for IPv4 Internet protocols
	// htonl converts a long integer (e.g. address) to a network representation
	// htons converts a short integer (e.g. port) to a network representation
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any address = 0.0.0.0
	host_addr.sin_port = htons(PORT);
	if (bind(_listen_fd, (struct sockaddr *) &host_addr, host_addrlen) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	// SOMAXCONN = maximum number of pending connections queued up before connections are refused
	if (listen(_listen_fd, SOMAXCONN) < 0)
	{
		perror("in listen");
		exit(EXIT_FAILURE);
	}
	printf("server listening for connections\n");

	while (1) {
		// Accept incoming connections
		int newsockfd = accept(_listen_fd, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);
		// host_addrlen is set to number of bytes of data actually stored by the kernel in socket address structure
		if (newsockfd < 0) {
			perror("webserver (accept)");
			continue;
		}
		printf("connection accepted\n");

		close(newsockfd);
	}
}


void ServerManager::pollSockets() {

}


void ServerManager::handleRequests(int client_fd) {

}

ServerManager::~ServerManager() {

}

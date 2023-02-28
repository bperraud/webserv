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

	struct sockaddr_in address;
	/* htonl converts a long integer (e.g. address) to a network representation */
	/* htons converts a short integer (e.g. port) to a network representation */
	memset((char *)&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(PORT);
	if (bind(_listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(_listen_fd, BACKLOG) < 0)
	{
		perror("In listen");
		exit(EXIT_FAILURE);
	}
}


void ServerManager::pollSockets() {

}

void ServerManager::handleNewConnections() {

}

void ServerManager::handleRequests(int client_fd) {

}

ServerManager::~ServerManager() {

}

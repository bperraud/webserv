#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Config.hpp"
#include "HttpHandler.hpp"

#include <iostream>
#include <errno.h>		// errno
#include <map>
#include <utility>		// pair
#include <string>
#include <cstring> 		//memset
#include <stdio.h>		// perror
#include <sys/socket.h>	// socket, bind, accept...
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h> 	// inet_ntoa
#include <sys/epoll.h>  // epoll
#include <fcntl.h>		// fcntl
#include <netinet/tcp.h>	// TCP_NODELAY


# define PORT 8080
# define BUFFER_SIZE 1024
# define MAX_EVENTS 50
# define MAX_CLIENT 10


class ServerManager {

private:
	std::map<int, HttpHandler*> _client_map;

    int		_listen_fd;
	int		_epoll_fd;
	struct sockaddr_in _host_addr;
	int	_host_addrlen;

public:
    ServerManager(Config config);
    ~ServerManager();

    void run();

	void setNonBlockingMode(int socket);
	void setupSocket();

	int	readFromClient(int client_fd);
	int	writeToClient(int client_fd);

	void connectionCloseMode(int client_fd);

	bool isEOF(const std::string &str);

    void initializeServer();
	void pollSockets();
    void handleNewConnections();
	void handleNewConnectionsEpoll();

    void handleRequests(int client_fd);
    void sendFile(int client_fd, const std::string &path);
    void sendDirectoryListing(int client_fd, const std::string &path);
    void sendErrorResponse(int client_fd, int status_code);
    void closeClientConnection(int client_fd);
};

#endif

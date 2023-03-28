#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Config.hpp"
#include "HttpHandler.hpp"
#include "CGIExecutor.hpp"

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

#if (defined (LINUX) || defined (__linux__))
#include <sys/epoll.h>  // epoll
#else
#include <sys/event.h>  // kqueue
#endif

#include <fcntl.h>		// fcntl
#include <netinet/tcp.h>	// TCP_NODELAY

# define PORT 8080
# define BUFFER_SIZE 1024
# define MAX_EVENTS 4096


class ServerManager {

private:
	std::map<int, HttpHandler*> _client_map;

    int		_listen_fd;

	CGIExecutor		_cgi_executor;
	#if (defined (LINUX) || defined (__linux__))
	int		_epoll_fd;
	#else
	int		_kqueue_fd;
	#endif
	struct sockaddr_in _host_addr;
	int		_host_addrlen;

public:
    ServerManager(Config config, CGIExecutor cgi);
    ~ServerManager();

    void run();

	void setNonBlockingMode(int socket);
	void setupSocket();

	int	readFromClient(int client_fd);
	int	writeToClient(int client_fd, const std::string &str);

	void connectionCloseMode(int client_fd);

	void handleNewConnections();

    void sendDirectoryListing(int client_fd, const std::string &path);
    void sendErrorResponse(int client_fd, int status_code);
    void closeClientConnection(int client_fd);
};

#endif

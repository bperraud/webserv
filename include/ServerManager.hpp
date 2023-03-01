#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Config.hpp"
//#include "Server.hpp"

#include <errno.h>		// errno
#include <cstring>
#include <string>
#include <stdio.h>		// perror
#include <sys/socket.h>	// socket, bind, accept...
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <poll.h>

# define PORT 8080
# define BUFFER_SIZE 1024

class ServerManager {

private:
    //Server *server;
    int		_listen_fd;
    pollfd	_poll_fds[1];

public:
    ServerManager(Config config);
    ~ServerManager();

    void run();

	void setupSocket();

    void initializeServer();
	void pollSockets();
    void handleNewConnections();
    void handleRequests(int client_fd);
    void sendFile(int client_fd, const std::string &path);
    void sendDirectoryListing(int client_fd, const std::string &path);
    void sendErrorResponse(int client_fd, int status_code);
    void closeClientConnection(int client_fd);
};
#endif

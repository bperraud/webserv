#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Config.hpp"
//#include "Server.hpp"

#include <cstring>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <poll.h>

# define PORT 8080
# define BACKLOG 10 // maximum number of pending connections that can be queued up before connections are refused

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

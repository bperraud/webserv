#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "HttpHandler.hpp"
#include "ServerConfig.hpp"

#include <iostream>
#include <errno.h>
#include <map>
#include <utility>		 // pair
#include <string>
#include <cstring> 		 // memset
#include <stdio.h>		 // perror
#include <sys/socket.h>	 // socket, bind, accept...
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>	 // inet_ntoa
#include <cassert>
#include <fcntl.h>		 // fcntl
#include <netinet/tcp.h> // TCP_NODELAY

#if (defined (LINUX) || defined (__linux__))
#include <sys/epoll.h>  // epoll
#else
#include <sys/event.h>  // kqueue
#endif

# define BUFFER_SIZE 4096	// min 4 bytes
# define MAX_EVENTS 4096
# define TIMEOUT_SECS 5
# define WAIT_TIMEOUT_SECS 2

struct server : public server_config {
	int	listen_fd;

    server(const server_config& info) : server_config(info) {};
};

typedef std::map<int, HttpHandler*> map_type;
typedef std::map<int, HttpHandler*>::iterator map_iterator_type;
typedef std::list<server>::const_iterator server_iterator_type;

class ServerManager {

private:
	std::list<server>	_server_list;
	map_type			_client_map;

#if (defined (LINUX) || defined (__linux__))
	int		_epoll_fd;
#else
	int		_kqueue_fd;
#endif

public:
    ServerManager(const ServerConfig &config);
    ~ServerManager();

    void	run();
	void	setNonBlockingMode(int socket);
	void	setupSocket(server &serv);

	int		readFromClient(int client_fd);
	void	writeToClient(int client_fd, const std::string &str);

	void	epollInit();
	const server*	isPartOfListenFd(int fd) const;

	void	handleReadEvent(int client_fd);
	void	handleWriteEvent(int client_fd);
	void 	handleNewConnection(int listen_fd, const server* serv);

	void	eventManager();
	void	timeoutCheck();
	void	connectionCloseMode(int client_fd);
    void	closeClientConnection(int client_fd);
	void	closeClientConnection(int client_fd, map_iterator_type elem);
};

#endif

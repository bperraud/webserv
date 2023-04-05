#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "HttpHandler.hpp"
#include "CGIExecutor.hpp"
#include "ServerConfig.hpp"

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
#include <cassert>


#if (defined (LINUX) || defined (__linux__))
#include <sys/epoll.h>  // epoll
#else
#include <sys/event.h>  // kqueue
#endif

#include <fcntl.h>		// fcntl
#include <netinet/tcp.h>	// TCP_NODELAY

# define BUFFER_SIZE 1024
# define MAX_EVENTS 4096
# define TIMEOUT_SECS 5
# define WAIT_TIMEOUT_SECS 2

typedef std::map<int, HttpHandler*> map_type;
typedef std::map<int, HttpHandler*>::iterator map_iterator_type;

struct server : public server_info {
	int					listen_fd;
	//map_type			server_client_map;

    server(const server_info& info) :
        server_info(info)
        //server_client_map()
	{ };
};

class ServerManager {

private:
	std::list<server>	_server_list;
	map_type			_client_map;
	CGIExecutor			_cgi_executor;

#if (defined (LINUX) || defined (__linux__))
	int		_epoll_fd;
#else
	int		_kqueue_fd;
#endif

public:
    ServerManager(ServerConfig config, CGIExecutor cgi);
    ~ServerManager();

    void	run();

	void	setNonBlockingMode(int socket);
	void	setupSocket();
	void	setupSocket(server &serv);

	int		readFromClient(int client_fd);
	void	writeToClient(int client_fd, const std::string &str);

	void	epollInit();
	bool	isPartOfListenFd(int fd) const;

	void	handleReadEvent(int client_fd);
	void	handleWriteEvent(int client_fd);
	void	handleNewConnections();

	void	timeoutCheck();
	void	connectionCloseMode(int client_fd);
    void	closeClientConnection(int client_fd);
	void	closeClientConnection(int client_fd, map_iterator_type elem);
};

#endif

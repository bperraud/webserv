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

# define BUFFER_SIZE 1024
# define MAX_EVENTS 4096
# define TIMEOUT_SECS 5
# define WAIT_TIMEOUT_SECS 2

typedef std::map<int, HttpHandler*> map_type;
typedef std::map<int, HttpHandler*>::iterator map_iterator_type;

class ServerManager {

private:
	map_type			_client_map;
    int					_listen_fd;
	int					_tfd;
	int					_PORT;
	std::string			_host;
	struct sockaddr_in	_host_addr;
	int					_host_addrlen;

	int					_max_body_size;

	bool				_auto_index;

	//server_names_type	_server_names;

	CGIExecutor			_cgi_executor;

	#if (defined (LINUX) || defined (__linux__))
	int		_epoll_fd;
	#else
	int		_kqueue_fd;
	#endif

public:
    ServerManager(Config config, CGIExecutor cgi);
    ~ServerManager();

    void run();

	void setNonBlockingMode(int socket);
	void setupSocket();

	int	readFromClient(int client_fd);
	int	writeToClient(int client_fd, const std::string &str);


	void handleReadEvent(int client_fd);
	void handleNewConnections();


	void connectionCloseMode(int client_fd);
    void closeClientConnection(int client_fd);
	void closeClientConnection(int client_fd, map_iterator_type elem);
};

#endif

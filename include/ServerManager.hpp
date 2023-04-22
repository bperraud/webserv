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
#include <csignal>
#include <stack>

#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define BLACK	"\033[1;30m"

#if (defined (LINUX) || defined (__linux__))
#include <sys/epoll.h>  // epoll
#else
#include <sys/event.h>  // kqueue
#endif

# define BUFFER_SIZE 4096
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

#if (defined (LINUX) || defined (__linux__))
# define INIT (fd) { \
	_epoll_fd = epoll_create1(0); \
	if (_epoll_fd == -1) { \
		throw std::runtime_error("epoll: create"); \
	} \
}
#else
# define INIT (fd) { \
	_kevent_fd = kqueue(); \
	if (_kevent_fd == -1) { \
		throw std::runtime_error("kqueue: create"); \
	} \
}
#endif

#if (defined (LINUX) || defined (__linux__))
# define DEL_EVENT (fd) { \
	struct epoll_event event; \
	event.data.fd = fd; \
	event.events = EPOLLIN | EPOLLET; \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &event) == -1) { \
		throw std::runtime_error("epoll_ctl: delete"); \
	} \
}
#else
# define DEL_EVENT (fd) { \
	struct kevent event; \
	EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL); \
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) == -1) { \
		throw std::runtime_error("kevent: delete"); \
	} \
}
#endif

#if (defined (LINUX) || defined (__linux__))
# define MOD_READ (fd) { \
	struct epoll_event event; \
	event.data.fd = fd; \
	event.events = EPOLLIN; \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) { \
		throw std::runtime_error("epoll_ctl: add"); \
	} \
}
#else
# define MOD_READ (fd) { \
	struct kevent event; \
	EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL); \
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) == -1) { \
		throw std::runtime_error("kevent: add"); \
	} \
}
#endif

#if (defined (LINUX) || defined (__linux__))
# define MOD_WRITE (fd) { \
	struct epoll_event event; \
	event.data.fd = fd; \
	event.events = EPOLLOUT; \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) { \
		throw std::runtime_error("epoll_ctl: mod"); \
	} \
}
#else
# define MOD_WRITE (fd) { \
	struct kevent event; \
	EV_SET(&event, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL); \
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) == -1) { \
		throw std::runtime_error("kevent: add"); \
	} \
}
#endif

class ServerManager {

private:
	std::list<server>	_server_list;
	map_type			_client_map;

#if (defined (LINUX) || defined (__linux__))
	int		_epoll_fd;
#else
	int		_kqueue_fd;
#endif

typedef std::pair<int, HttpHandler*> fd_client_pair;

public:
    ServerManager(const ServerConfig &config);
    ~ServerManager();

    void	run();
	void	setNonBlockingMode(int socket);
	void	setupSocket(server &serv);

	void	printServerSocket(int socket);

	void	epollInit();
	const server*	isPartOfListenFd(int fd) const;

	int 	treatReceiveData(char *buffer, const ssize_t nbytes, HttpHandler *client);
	int		readFromClient(fd_client_pair client);
	void	writeToClient(int client_fd, const std::string &str);

	void	handleReadEvent(fd_client_pair client);
	void	handleWriteEvent(fd_client_pair client);
	void 	handleNewConnection(int listen_fd, const server* serv);

	void	eventManager();
	void	timeoutCheck();
	void	connectionCloseMode(fd_client_pair client);
    void	closeClientConnection(fd_client_pair client);
	void	closeClientConnection(fd_client_pair client, map_iterator_type elem);

	void	clear_client_map();
};

#endif

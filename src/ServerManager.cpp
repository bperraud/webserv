#include "ServerManager.hpp"


ServerManager::ServerManager(ServerConfig config, CGIExecutor cgi) : _cgi_executor(cgi) {
	std::list<server_config> server_list = config.getServerList();
	for (std::list<server_config>::iterator it = server_list.begin(); it != server_list.end(); ++it) {
		std::cout << "server manager : " << *it << std::endl;
		server serv(*it);
		setupSocket(serv);
		_server_list.push_back(serv);
	}
	epollInit();
}

void ServerManager::run() {
	eventManager();
}

void	ServerManager::setupSocket(server &serv) {
	struct sockaddr_in host_addr;
	serv.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (serv.listen_fd == 0)
		throw std::runtime_error("cannot create socket");
	memset((char *)&host_addr, 0, sizeof(host_addr));
	int host_addrlen = sizeof(host_addr);
	host_addr.sin_family = AF_INET; // AF_INET for IPv4 Internet protocols
	host_addr.sin_addr.s_addr = inet_addr(serv.host.c_str());
	host_addr.sin_port = htons(serv.PORT);
	int enable_reuseaddr = 1;
	if (setsockopt(serv.listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuseaddr, sizeof(int)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
	// disables the Nagle algorithm, which can improve performance for small messages,
	//but can degrade performance for large messages or bulk data transfer.
	int enable_nodelay = 1;
	if (setsockopt(serv.listen_fd, IPPROTO_TCP, TCP_NODELAY, &enable_nodelay, sizeof(int)) < 0)
		throw std::runtime_error("setsockopt(TCP_NODELAY) failed");
	if (bind(serv.listen_fd, (struct sockaddr *) &host_addr, host_addrlen) < 0)
		throw std::runtime_error("bind failed");
	setNonBlockingMode(serv.listen_fd);
	if (listen(serv.listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen failed");
	std::cout << "server listening for connections... " << std::endl;
}

void ServerManager::setNonBlockingMode(int socket) {
	if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("failed to set socket to non-blocking mode");
}

void ServerManager::timeoutCheck() {
	for (map_iterator_type it = _client_map.begin(); it != _client_map.end();) {
		if (it->second->hasTimeOut()) {
			int fd = it->first;
			map_iterator_type to_delete = it;
			++it;
			closeClientConnection(fd, to_delete);
		}
		else
			++it;
	}
}

const server*	ServerManager::isPartOfListenFd(int fd) const {
	for (server_iterator_type serv = _server_list.begin(); serv != _server_list.end(); ++serv) {
		if (fd == serv->listen_fd)
			return serv.operator->();
	}
	return NULL;
}

#if (defined (LINUX) || defined (__linux__))
void ServerManager::epollInit() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd < 0)
		throw std::runtime_error("epoll_create1");
	struct epoll_event event;
	for (std::list<server>::iterator serv = _server_list.begin(); serv != _server_list.end(); ++serv) {
		event.data.fd = serv->listen_fd;
		event.events = EPOLLIN;
		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, serv->listen_fd, &event) < 0)
			throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD");
	}
}

void ServerManager::handleNewConnection(int socket, const server *serv) {
	struct epoll_event event;
	struct sockaddr_in client_addr;
	socklen_t client_addrlen = sizeof(client_addr);
	int new_sockfd = accept(socket, (struct sockaddr *)&client_addr, &client_addrlen);
	if (new_sockfd < 0)
		throw std::runtime_error("accept()");
	setNonBlockingMode(new_sockfd);
	event.data.fd = new_sockfd;
	event.events = EPOLLIN | EPOLLOUT;;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_sockfd, &event) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD");
	_client_map.insert(std::make_pair(new_sockfd, new HttpHandler(TIMEOUT_SECS, serv)));
	std::cout << "new connection accepted for client on socket : " << new_sockfd << std::endl;
}

void ServerManager::eventManager() {
	std::vector<struct epoll_event> events(MAX_EVENTS);
	while (1) {
		int n_ready = epoll_wait(_epoll_fd, events.data(), MAX_EVENTS, WAIT_TIMEOUT_SECS * 1000);
		if (n_ready == -1)
			throw std::runtime_error("epoll_wait");
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			const server* serv = isPartOfListenFd(fd);
			if (serv) {
				handleNewConnection(fd, serv);
			}
			else if (events[i].events & EPOLLIN) {
				handleReadEvent(fd);
			}
			else if (events[i].events & EPOLLOUT) {
				handleWriteEvent(fd);
			}
		}
		timeoutCheck();
	}
}
#else

void ServerManager::epollInit() {
	_kqueue_fd = kqueue();
	if (_kqueue_fd < 0)
		throw std::runtime_error("kqueue");
	struct kevent event;
	for (std::list<server>::iterator serv = _server_list.begin(); serv != _server_list.end(); ++serv) {
		EV_SET(&event, serv->listen_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
			throw std::runtime_error("kevent EV_ADD");
	}
}

void ServerManager::handleNewConnection(int socket, const server *serv) {
	struct kevent event;
	struct sockaddr_in client_addr;
	socklen_t client_addrlen = sizeof(client_addr);
	int new_sockfd = accept(socket, (struct sockaddr *)&client_addr, &client_addrlen);
	if (new_sockfd < 0)
		throw std::runtime_error("accept()");
	setNonBlockingMode(new_sockfd);
	EV_SET(&event, new_sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent add read");
	EV_SET(&event, new_sockfd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent add write");
	_client_map.insert(std::make_pair(new_sockfd, new HttpHandler(TIMEOUT_SECS, serv)));
	std::cout << "new connection accepted for client on socket : " << new_sockfd << std::endl;
}

void ServerManager::eventManager() {
	struct kevent events[MAX_EVENTS];
	while (1) {
		struct timespec timeout;
		timeout.tv_sec = WAIT_TIMEOUT_SECS;
		timeout.tv_nsec = 0;
		int n_ready = kevent(_kqueue_fd, NULL, 0, events, MAX_EVENTS, &timeout);
		if (n_ready == -1)
			throw std::runtime_error("kevent wait");
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].ident;
			const server* serv = isPartOfListenFd(fd);
			if (serv) {
				handleNewConnection(fd, serv);
			}
			else if (events[i].filter == EVFILT_READ) {
				handleReadEvent(fd);
			}
			else if (events[i].filter == EVFILT_WRITE) {
				handleWriteEvent(fd);
			}
		}
		timeoutCheck();
	}
}
#endif

void ServerManager::handleReadEvent(int client_fd) {
	if (!readFromClient(client_fd)) {
		HttpHandler *client = _client_map[client_fd];
		client->stopTimer();

		#if 1
		std::cout << "Header :" << std::endl;
		std::cout << client->getRequest() << std::endl;
		#else
		std::cout << "Message body :" << std::endl;
		std::cout << client->getBody() << std::endl;
		#endif

		client->createHttpResponse();

		if (client->isCGIMode())
		{
			std::cout << "Header :" << std::endl;
			std::cout << client->getRequest() << std::endl;
			std::cout << "Message body :" << std::endl;
			std::cout << client->getBody() << std::endl;
			_cgi_executor.run(client->getStructRequest(), client_fd);
		}
		else{

		#if 1
		std::cout << "Response Header :" << std::endl;
		std::cout << client->getResponseHeader() << std::endl;
		std::cout << "Response Message body :" << std::endl;
		std::cout << client->getResponseBody() << std::endl;
		#endif

		}
		client->setReadyToWrite(true);
	}
}

void ServerManager::handleWriteEvent(int client_fd) {
	HttpHandler *client = _client_map[client_fd];
	if (client->isReadyToWrite())
	{
		writeToClient(client_fd, client->getResponseHeader());
		writeToClient(client_fd, client->getResponseBody());
		client->resetStream();
		connectionCloseMode(client_fd);
		client->setReadyToWrite(false);
	}
}


void ServerManager::closeClientConnection(int client_fd) {
#if (defined (LINUX) || defined (__linux__))
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_DEL");
#else
	struct kevent events[2];
	EV_SET(events, client_fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(events + 1, client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	if (kevent(_kqueue_fd, events, 2, NULL, 0, NULL) < 0) {
		throw std::runtime_error("kevent delete");
	}
#endif
	delete _client_map[client_fd];
	_client_map.erase(client_fd);
	close(client_fd);
	std::cout << "connection closed on client " << client_fd << std::endl;
	return ;
}

void ServerManager::closeClientConnection(int client_fd, map_iterator_type elem) {
#if (defined (LINUX) || defined (__linux__))
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_DEL");
#else
	struct kevent events[2];
	EV_SET(events, client_fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(events + 1, client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	if (kevent(_kqueue_fd, events, 2, NULL, 0, NULL ) < 0) {
		throw std::runtime_error("kevent delete");
	}
#endif
	delete _client_map[client_fd];
	_client_map.erase(elem);
	close(client_fd);
	std::cout << "connection closed on client " << client_fd << std::endl;
	return ;
}

void ServerManager::connectionCloseMode(int client_fd) {
	if (!_client_map[client_fd]->isKeepAlive())
		closeClientConnection(client_fd);
}

int	ServerManager::readFromClient(int client_fd){
	char buffer[BUFFER_SIZE + 4];

	HttpHandler *client = _client_map[client_fd];
	const ssize_t nbytes = recv(client_fd, buffer + 4, BUFFER_SIZE, 0);
	client->startTimer();
	client->copyLast4Char(buffer, nbytes);
	if (nbytes == -1)
		throw std::runtime_error("recv()");
	else if (nbytes == 0) {
		closeClientConnection(client_fd);
		return 1;
	}
	else {
		std::cout << "finished reading data from client " << client_fd << std::endl;
		ssize_t body_left_to_read = client->getLeftToRead();
		if (body_left_to_read > 0)
		{
			body_left_to_read = client->writeToBody(buffer + 4, nbytes);
			return (body_left_to_read > 0);
		}
		size_t pos_end_header = ((std::string)buffer).find(CRLF);
		if (pos_end_header == std::string::npos) {
			client->writeToStream(buffer + 4, nbytes);
			return 1;
		}
		else {
			client->writeToStream(buffer + 4, pos_end_header);
			client->parseRequest();
			body_left_to_read = client->writeToBody(buffer + 4 + pos_end_header, nbytes - pos_end_header);
			return (body_left_to_read > 0);
		}
	}
	return 1;
}

void ServerManager::writeToClient(int client_fd, const std::string &str) {
	ssize_t nbytes = send(client_fd, str.c_str(), str.length(), 0);
	if (nbytes == -1)
		throw std::runtime_error("send()");
	else if ((size_t) nbytes == str.length()) {
		std::cout << "finished writing " << client_fd << std::endl;
	}
	else {
		std::cout << "writing " << nbytes << " bytes to client " << client_fd << std::endl;
		writeToClient(client_fd, str.substr(nbytes));
	}
}

ServerManager::~ServerManager() {
	for (server_iterator_type serv = _server_list.begin(); serv != _server_list.end(); ++serv) {
		close(serv->listen_fd);
	}
	for (map_iterator_type it = _client_map.begin(); it != _client_map.end(); it++) {
		delete it->second;
	}
}

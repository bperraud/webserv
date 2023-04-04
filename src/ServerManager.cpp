#include "ServerManager.hpp"


ServerManager::ServerManager(Config config, CGIExecutor cgi) : _cgi_executor(cgi) {
	(void) config;
	_PORT = 8080;
	_host = "0.0.0.0";
}

void ServerManager::run() {
	setupSocket();
	handleNewConnections();
}

void	ServerManager::setupSocket() {
	_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_fd < 0)
		throw std::runtime_error("cannot create socket");
	memset((char *)&_host_addr, 0, sizeof(_host_addr));
	int _host_addrlen = sizeof(_host_addr);
	_host_addr.sin_family = AF_INET;				// AF_INET for IPv4 Internet protocols
	//_host_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any address = 0.0.0.0

	_host_addr.sin_addr.s_addr = inet_addr(_host.c_str());
	_host_addr.sin_port = htons(_PORT);
	int enable_reuseaddr = 1;
	if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuseaddr, sizeof(int)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
	// disables the Nagle algorithm, which can improve performance for small messages,
	//but can degrade performance for large messages or bulk data transfer.
	int enable_nodelay = 1;
	if (setsockopt(_listen_fd, IPPROTO_TCP, TCP_NODELAY, &enable_nodelay, sizeof(int)) < 0)
		throw std::runtime_error("setsockopt(TCP_NODELAY) failed");
	if (bind(_listen_fd, (struct sockaddr *) &_host_addr, _host_addrlen) < 0)
		throw std::runtime_error("bind failed");
	setNonBlockingMode(_listen_fd);
	// SOMAXCONN = maximum number of pending connections queued up before connections are refused
	if (listen(_listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen failed");
	printf("server listening for connections...\n");
}

void ServerManager::setNonBlockingMode(int socket) {
	if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0) {
		perror("Failed to set socket to non-blocking mode");
		close(socket);
	}
}

void ServerManager::timeoutCheck() {
	for (map_iterator_type it = _client_map.begin(); it != _client_map.end();) {
		if (it->second->hasTimeOut()) {
			int fd = it->first;
			map_iterator_type to_delete = it;
			++it;
			closeClientConnection(fd, to_delete);
		}
		else {
			++it;
		}
	}
}

#if (defined (LINUX) || defined (__linux__))
void ServerManager::handleNewConnections() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd < 0)
		throw std::runtime_error("epoll_create1");
	// Add the listen socket to the epoll interest list
	struct epoll_event event;
	event.data.fd = _listen_fd;
	event.events = EPOLLIN;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _listen_fd, &event) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD");
	struct epoll_event events[MAX_EVENTS];
	// events array is used to store events that occur on any of
	// the file descriptors that have been registered with epoll_ctl()

	while (1) {
		std::cout << "waiting..." << std::endl;
		int n_ready = epoll_wait(_epoll_fd, events, MAX_EVENTS, WAIT_TIMEOUT_SECS * 1000);
		// if any of the file descriptors match the interest then epoll_wait can return without blocking.
		if (n_ready == -1)
			throw std::runtime_error("epoll_wait");
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			// If the listen socket is ready, accept a new connection and add it to the epoll interest list
			if (fd == _listen_fd) {
				struct sockaddr_in client_addr;
				socklen_t client_addrlen = sizeof(client_addr);
				int newsockfd = accept(_listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
				if (newsockfd < 0)
					throw std::runtime_error("accept()");
				setNonBlockingMode(newsockfd);
				event.data.fd = newsockfd;
				event.events = EPOLLIN ;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) < 0)
					throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD");
				_client_map.insert(std::make_pair(newsockfd, new HttpHandler(TIMEOUT_SECS)));
				std::cout << "new connection accepted for client on socket : " << newsockfd << std::endl;
			}
			else if ( events[i].events & EPOLLIN ) {
				handleReadEvent(fd);
			}

			else {
				;
			}
		}

		timeoutCheck();
	}
}
#else
void ServerManager::handleNewConnections() {
	_kqueue_fd = kqueue();
	if (_kqueue_fd == -1)
		throw std::runtime_error("kqueue");
	struct kevent event;
	EV_SET(&event, _listen_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent");
	struct kevent events[MAX_EVENTS];
	while (1) {
		struct timespec timeout;
		timeout.tv_sec = WAIT_TIMEOUT_SECS;
		timeout.tv_nsec = 0;

		std::cout << "waiting..." << std::endl;
		int n_ready = kevent(_kqueue_fd, NULL, 0, events, MAX_EVENTS, &timeout);
		if (n_ready == -1)
			throw std::runtime_error("kevent");
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].ident;
			if (fd == _listen_fd) {
				struct sockaddr_in client_addr;
				socklen_t client_addrlen = sizeof(client_addr);
				int newsockfd = accept(_listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
				if (newsockfd < 0)
					throw std::runtime_error("accept()");
				setNonBlockingMode(newsockfd);
				// set SO_LINGER socket option with a short timeout value
				struct linger l;
				l.l_onoff = 1;
				l.l_linger = 0; // no timeout for closing socket
				setsockopt(newsockfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
				EV_SET(&event, newsockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
				if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
					throw std::runtime_error("kevent");
				_client_map.insert(std::make_pair(newsockfd, new HttpHandler(TIMEOUT_SECS)));
				std::cout << "new connection accepted for client on socket : " << newsockfd << std::endl;
			}
			else if (_events[i].filter ==  EVFILT_READ) {
			// else
				handleReadEvent(fd);
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

		#if 0
		std::cout << "Response Header :" << std::endl;
		std::cout << client->getResponseHeader() << std::endl;
		std::cout << "Response Message body :" << std::endl;
		std::cout << client->getResponseBody() << std::endl;
		#endif

		writeToClient(client_fd, client->getResponseHeader());
		writeToClient(client_fd, client->getResponseBody());

		}
		client->resetStream();
		connectionCloseMode(client_fd);
	}
}

#if (defined (LINUX) || defined (__linux__))
void ServerManager::closeClientConnection(int client_fd) {
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_DEL");
#else
void ServerManager::closeClientConnection(int client_fd) {
    struct kevent event;
    EV_SET(&event, client_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent");
#endif
	delete _client_map[client_fd];
	_client_map.erase(client_fd);
	close(client_fd);
	printf("connection closed on client %d\n", client_fd);
	return ;
}

#if (defined (LINUX) || defined (__linux__))
void ServerManager::closeClientConnection(int client_fd, map_iterator_type elem) {
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_DEL");
#else
void ServerManager::closeClientConnection(int client_fd, map_iterator_type elem) {
    struct kevent event;
    EV_SET(&event, client_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
        throw std::runtime_error("kevent");
#endif
	delete _client_map[client_fd];
	_client_map.erase(elem);
	close(client_fd);
	printf("connection closed on client %d\n", client_fd);
	return ;
}

void ServerManager::connectionCloseMode(int client_fd) {
	if (!_client_map[client_fd]->isKeepAlive())
		closeClientConnection(client_fd);
}

int	ServerManager::readFromClient(int client_fd){
	char buffer[BUFFER_SIZE + 4];
	HttpHandler *client = _client_map[client_fd];

	ssize_t nbytes = recv(client_fd, buffer + 4, BUFFER_SIZE, 0);
	client->startTimer();
	client->copyLast4Char(buffer, nbytes);
	if (nbytes == -1) {
		perror("recv()");
		closeClientConnection(client_fd);
		return 1;
	}
	else if (nbytes == 0) {
		closeClientConnection(client_fd);
		return 1;
	}
	else {
		printf("finished reading data from client %d\n", client_fd);
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

int ServerManager::writeToClient(int client_fd, const std::string &str) {
	ssize_t nbytes = send(client_fd, str.c_str(), str.length(), 0);
	if (nbytes == -1)
		throw std::runtime_error("send()");
	else if ((size_t) nbytes == str.length()) {
		printf("finished writing %d\n", client_fd);
	}
	return 0;
}

ServerManager::~ServerManager() {
	close(_listen_fd);
	for (map_iterator_type it = _client_map.begin(); it != _client_map.end(); it++) {
		delete it->second;
	}
}

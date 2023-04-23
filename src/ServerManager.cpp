#include "ServerManager.hpp"


const int* ServerManager::hostLevel(int port)  {

	struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
	for (fd_port_level1::iterator it = _list_server_map.begin(); it != _list_server_map.end(); ++it) {

		std::cout << "fd: " << it->first << std::endl;
		int status = getsockname(it->first, (struct sockaddr *)&addr, &addrlen);
		if (status != 0) {
			throw std::runtime_error("getsockname error");
		}
		char address_str[INET_ADDRSTRLEN];  // buffer to hold the human-readable address
		inet_ntop(AF_INET, &(addr.sin_addr), address_str, INET_ADDRSTRLEN);

		std::cout << "Local port: " << ntohs(addr.sin_port) << std::endl;

		if (port == ntohs(addr.sin_port))
		{
			return &it->first;
		}
	}
	return NULL;
}

ServerManager::ServerManager(const ServerConfig &config) {

	std::list<server_config> server_list = config.getServerList();
	for (std::list<server_config>::iterator it = server_list.begin(); it != server_list.end(); ++it) {
		server serv(*it);
		const int* server_fd = hostLevel(serv.PORT);
		if ( server_fd ) { // port exist on the config
			//check for level 3
			serv.listen_fd = *server_fd;
			if (_list_server_map[*server_fd].find(serv.host) != _list_server_map[*server_fd].end()) {
				server_name_level3 server_name_map = _list_server_map[*server_fd][serv.host];
				if (server_name_map.find(serv.name) != server_name_map.end()) {
					throw std::runtime_error("server name already exist");
				}
				server_name_map.insert(std::make_pair(serv.name, serv)); // update level 3
			}
			_list_server_map[*server_fd].insert(std::make_pair(serv.host, server_name_level3())); // update level 2
			_list_server_map[*server_fd][serv.host].insert(std::make_pair(serv.name, serv)); // update level 3
		}
		else {	// new host
			setupSocket(serv);
			_list_server_map.insert(std::make_pair(serv.listen_fd, host_level2())); //update level 1
			_list_server_map[serv.listen_fd].insert(std::make_pair(serv.host, server_name_level3())); //update level 2
			_list_server_map[serv.listen_fd][serv.host].insert(std::make_pair(serv.name, serv)); //update level 3
		}
	}
	epollInit();
}

void ServerManager::run() {
	eventManager();
}

void ServerManager::printServerSocket(int socket) const {
	std::cout << BLACK << "[" <<  socket  << "] " << RESET ;
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
	printServerSocket(serv.listen_fd);
	std::cout <<  "server listening for connections -> "
	<< YELLOW << "[" << serv.host << ", " << serv.PORT << "] " <<  RESET << std::endl;
}

void ServerManager::setNonBlockingMode(int socket) {
	if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("failed to set socket to non-blocking mode");
}

void ServerManager::timeoutCheck() {
	for (map_iterator_type it = _client_map.begin(); it != _client_map.end();) {
		if (it->second->hasTimeOut()) {
			fd_client_pair client = *it;
			map_iterator_type to_delete = it;
			++it;
			closeClientConnection(client, to_delete);
		}
		else
			++it;
	}
}


// return
 host_level2*	ServerManager::isPartOfListenFd(int fd)  {
	for (fd_port_level1::iterator serv_it = _list_server_map.begin(); serv_it != _list_server_map.end(); ++serv_it) {
		if (fd == serv_it->first)
			return &(serv_it->second);
	}
	return NULL;
}

#if (defined (LINUX) || defined (__linux__))
void ServerManager::epollInit() {
	_epoll_fd = epoll_create1(0);
	if (_epoll_fd < 0)
		throw std::runtime_error("epoll_create1");
	struct epoll_event event;
	for (fd_port_level1::const_iterator serv_it = _list_server_map.begin(); serv_it != _list_server_map.end(); ++serv_it) {
		event.data.fd = serv_it->first;
		event.events = EPOLLIN;
		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, serv_it->first, &event) < 0)
			throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD");
	}
}

void ServerManager::handleNewConnection(int socket, host_level2* host_map) {
	struct epoll_event event;
	struct sockaddr_in client_addr;
	socklen_t client_addrlen = sizeof(client_addr);
	int new_sockfd = accept(socket, (struct sockaddr *)&client_addr, &client_addrlen);
	if (new_sockfd < 0)
		throw std::runtime_error("accept()");


	getsockname( new_sockfd, (struct sockaddr *)( &client_addr ), &client_addrlen );

	std::ostringstream client_ip_stream;
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 0) & 0xFF) << ".";
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 8) & 0xFF) << ".";
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 16) & 0xFF) << ".";
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 24) & 0xFF);
	std::string client_ip = client_ip_stream.str();

	char client_port_str[6];
	sprintf(client_port_str, "%u", ntohs(client_addr.sin_port));
	std::string client_port = client_port_str;

	std::cout << "host : " << client_ip << std::endl;
	std::cout << "port : " << client_port << std::endl;

	setNonBlockingMode(new_sockfd);
	event.data.fd = new_sockfd;
	event.events = EPOLLIN | EPOLLOUT;;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, new_sockfd, &event) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD");
	_client_map.insert(std::make_pair(new_sockfd, new HttpHandler(TIMEOUT_SECS, &(*host_map)[client_ip])));
	std::cout << "new connection -> " <<  GREEN << "client " << new_sockfd << RESET << std::endl;
}

void ServerManager::eventManager() {
	struct epoll_event events[MAX_EVENTS];
	while (1) {
		int n_ready = epoll_wait(_epoll_fd, events, MAX_EVENTS, WAIT_TIMEOUT_SECS * 1000);
		if (n_ready == -1)
			throw std::runtime_error("epoll_wait");
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			host_level2* serv = isPartOfListenFd(fd);
			if (serv) {
				handleNewConnection(fd, serv);
			}
			else {
				fd_client_pair client = *_client_map.find(fd);
				if (events[i].events & EPOLLIN) {
					handleReadEvent(client);
				}
				else if (events[i].events & EPOLLOUT) {
					handleWriteEvent(client);
				}
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
	for (fd_port_level1::const_iterator serv_it = _list_server_map.begin(); serv_it != _list_server_map.end(); ++serv_it) {
		EV_SET(&event, serv_it->first, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
			throw std::runtime_error("kevent EV_ADD");
	}
}

void ServerManager::handleNewConnection(int socket, host_level2* host_map) {
	struct kevent event;
	struct sockaddr_in client_addr;
	socklen_t client_addrlen = sizeof(client_addr);
	int new_sockfd = accept(socket, (struct sockaddr *)&client_addr, &client_addrlen);
	if (new_sockfd < 0)
		throw std::runtime_error("accept()");

	getsockname( new_sockfd, (struct sockaddr *)( &client_addr ), &client_addrlen );

	std::ostringstream client_ip_stream;
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 0) & 0xFF) << ".";
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 8) & 0xFF) << ".";
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 16) & 0xFF) << ".";
	client_ip_stream << ((client_addr.sin_addr.s_addr >> 24) & 0xFF);
	std::string client_ip = client_ip_stream.str();

	char client_port_str[6];
	sprintf(client_port_str, "%u", ntohs(client_addr.sin_port));
	std::string client_port = client_port_str;

	std::cout << "host : " << client_ip << std::endl;
	std::cout << "port : " << client_port << std::endl;

	setNonBlockingMode(new_sockfd);
	EV_SET(&event, new_sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent add read");
	EV_SET(&event, new_sockfd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent add write");
	_client_map.insert(std::make_pair(new_sockfd, new HttpHandler(TIMEOUT_SECS, &(*host_map)[client_ip])));
	std::cout << "new connection -> " <<  GREEN << "client " << new_sockfd << RESET << std::endl;
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
			host_level2* serv = isPartOfListenFd(fd);
			if (serv) {
				handleNewConnection(fd, serv);
			}
			else {
				fd_client_pair client = *_client_map.find(fd);
				if (events[i].filter == EVFILT_READ) {
					handleReadEvent(client);
				}
				else if (events[i].filter == EVFILT_WRITE) {
					handleWriteEvent(client);
				}
			}
		}
		timeoutCheck();
	}
}
#endif

void ServerManager::handleReadEvent(fd_client_pair client)  {
	if (!readFromClient(client)) {
		client.second->stopTimer();

		#if 1
		std::cout << "Header for client : " << client.first << std::endl;
		std::cout << client.second->getRequest() << std::endl;
		std::cout << "Message body :" << std::endl;
		std::cout << client.second->getBody() << std::endl;
		#endif

		client.second->createHttpResponse();

		#if 0
		std::cout << "Response Header to client : " << lient.first << std::endl;
		std::cout << client.second->getResponseHeader() << std::endl;
		std::cout << "Response Message body to client : " << lient.first  << std::endl;
		std::cout << client.second->getResponseBody() << std::endl;
		#endif

		client.second->setReadyToWrite(true);
	}
}

void ServerManager::handleWriteEvent(fd_client_pair client) {
	if (client.second->isReadyToWrite())
	{
		writeToClient(client.first, client.second->getResponseHeader());
		writeToClient(client.first, client.second->getResponseBody());
		client.second->resetRequestContext();
		connectionCloseMode(client);
		client.second->setReadyToWrite(false);
	}
}

void ServerManager::closeClientConnection(fd_client_pair client) {
#if (defined (LINUX) || defined (__linux__))
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client.first, NULL) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_DEL");
#else
	struct kevent events[2];
	EV_SET(events, client.first, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(events + 1, client.first, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	if (kevent(_kqueue_fd, events, 2, NULL, 0, NULL) < 0) {
		throw std::runtime_error("kevent delete");
	}
#endif
	delete client.second;
	_client_map.erase(client.first);
	close(client.first);
	std::cout << "connection closed ->" << RED << " client " << client.first << RESET << std::endl;
}

void ServerManager::closeClientConnection(fd_client_pair client, map_iterator_type elem) {
#if (defined (LINUX) || defined (__linux__))
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client.first, NULL) < 0)
		throw std::runtime_error("epoll_ctl EPOLL_CTL_DEL");
#else
	struct kevent events[2];
	EV_SET(events, client.first, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(events + 1, client.first, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	if (kevent(_kqueue_fd, events, 2, NULL, 0, NULL ) < 0) {
		throw std::runtime_error("kevent delete");
	}
#endif
	delete client.second;
	_client_map.erase(elem);
	close(client.first);
	std::cout << "connection closed ->" << RED << " client " << client.first << RESET << std::endl;
}

void ServerManager::connectionCloseMode(fd_client_pair client) {
	if (!client.second->isKeepAlive())
		closeClientConnection(client);
}

int ServerManager::treatReceiveData(char *buffer, const ssize_t nbytes, HttpHandler *client) {
	client->startTimer();
	client->copyLast4Char(buffer, nbytes);
	bool isBodyUnfinished = client->isBodyUnfinished();
	if (isBodyUnfinished)
	{
		isBodyUnfinished = client->writeToBody(buffer + 4, nbytes);
		return (isBodyUnfinished);
	}
	const size_t pos_end_header = ((std::string)buffer).find(CRLF);
	if (pos_end_header == std::string::npos) {
		client->writeToStream(buffer + 4, nbytes);
		return 1;
	}
	else {
		client->writeToStream(buffer + 4, pos_end_header);
		client->resetLast4();
		client->parseRequest();
		isBodyUnfinished = client->writeToBody(buffer + 4 + pos_end_header, nbytes - pos_end_header);
		return (isBodyUnfinished);
	}
}

int	ServerManager::readFromClient(fd_client_pair client) {
	char buffer[BUFFER_SIZE + 4];
	const ssize_t nbytes = recv(client.first, buffer + 4, BUFFER_SIZE, 0);

	if (nbytes <= 0) {
		closeClientConnection(client);
		return 1;
	}
	else {
		return (treatReceiveData(buffer, nbytes, client.second));
	}
	return 1;
}

void ServerManager::writeToClient(int client_fd, const std::string &str) {
	const ssize_t nbytes = send(client_fd, str.c_str(), str.length(), 0);
	if (nbytes == -1)
		throw std::runtime_error("send()");
	else if ((size_t) nbytes < str.length()) {
		writeToClient(client_fd, str.substr(nbytes));
	}
}

ServerManager::~ServerManager() {
	//for (std::list<server>::iterator it = _server_list.begin(); it != _server_list.end(); ++it) {
	//	std::cout << "connection closed ->" << RED << " server " << it->listen_fd << RESET << std::endl;
	//	close(it->listen_fd);
	//}
	if (!_client_map.empty()) {
		std::stack<fd_client_pair> stack;
		for (map_iterator_type it = _client_map.begin(); it != _client_map.end(); ++it) {
			stack.push(*it);
		}
		size_t size = _client_map.size();
		for (size_t i = 0; i < size; ++i) {
			closeClientConnection(stack.top());
			stack.pop();
		}
	}
	#if (defined (LINUX) || defined (__linux__))
		close(_epoll_fd);
	#else
		close(_kqueue_fd);
	#endif
}

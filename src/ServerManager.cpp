#include "ServerManager.hpp"

const int* ServerManager::hostLevel(int port)  {
	struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
	for (auto &[fd, _port] : _list_server_map) {
		if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) != 0)
			throw std::runtime_error("getsockname error");
		char address_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(addr.sin_addr), address_str, INET_ADDRSTRLEN);
		if (port == ntohs(addr.sin_port))
			return &fd;
	}
	return NULL;
}

ServerManager::ServerManager(const ServerConfig &config) {
	std::list<server_config> server_list = config.getServerList();
	for (auto it = server_list.begin(); it != server_list.end(); ++it) {
		server serv(*it);
		serv.is_default = false;
		const int* server_fd = hostLevel(serv.PORT);
		if (server_fd) { // port exist on the config
			serv.listen_fd = *server_fd;
			if (_list_server_map[*server_fd].count(serv.host)) { // host exist on the config
				server_name_level3 server_name_map = _list_server_map[*server_fd][serv.host];
				if (server_name_map.count(serv.name))
					throw std::runtime_error("server name already exist");
				server_name_map.insert(std::make_pair(serv.name, serv));
			}
			_list_server_map[*server_fd].insert(std::make_pair(serv.host, server_name_level3()));
			serv.is_default = true;
			_list_server_map[*server_fd][serv.host].insert(std::make_pair(serv.name, serv));
		} else { // new socket
			setupSocket(serv);
			serv.is_default = true;
			_list_server_map.insert(std::make_pair(serv.listen_fd, host_level2()));
			_list_server_map[serv.listen_fd].insert(std::make_pair(serv.host, server_name_level3()));
			_list_server_map[serv.listen_fd][serv.host].insert(std::make_pair(serv.name, serv));
		}
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
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	host_addr.sin_port = htons(serv.PORT);
	int enable_reuseaddr = 1;
	if (setsockopt(serv.listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuseaddr, sizeof(int)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
	int enable_nodelay = 1;
	if (setsockopt(serv.listen_fd, IPPROTO_TCP, TCP_NODELAY, &enable_nodelay, sizeof(int)) < 0)
		throw std::runtime_error("setsockopt(TCP_NODELAY) failed");
	if (bind(serv.listen_fd, (struct sockaddr *) &host_addr, host_addrlen) < 0)
		throw std::runtime_error("bind failed");
	setNonBlockingMode(serv.listen_fd);
	if (listen(serv.listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen failed");
	std::cout << BLACK << "[" <<  serv.listen_fd  << "] " << RESET ;
	std::cout << "server listening for connections -> "
	<< YELLOW << "[" << serv.host << ", " << serv.PORT << "] " <<  RESET << std::endl;
}

void ServerManager::setNonBlockingMode(int socket) {
	if (fcntl(socket, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("failed to set socket to non-blocking mode");
}

void ServerManager::timeoutCheck() {
	for (auto it = _client_map.begin(); it != _client_map.end(); ) {
        auto currentIt = it++;
		if (currentIt->second->HasTimeOut())
		{
			std::cout << "timeout on -> " << RED << " client " << currentIt->first << RESET << std::endl;
			closeClientConnection(*currentIt);
		}
    }
}

host_level2* ServerManager::isPartOfListenFd(int fd)  {
	for (auto &[_fd, port] : _list_server_map) {
		if (fd == _fd)
			return &(port);
	}
	return NULL;
}

void ServerManager::epollInit() {
	INIT_EPOLL;
	for (auto &[fd, port] : _list_server_map) {
		MOD_READ(fd);
	}
}

void ServerManager::handleNewConnection(int socket, host_level2* host_map) {
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
	std::string client_port_str;
	std::stringstream ss;
	ss << ntohs(client_addr.sin_port);
	ss >> client_port_str;
	std::string client_port = client_port_str;
	server_name_level3 *server_name ;
	if (host_map->find(client_ip) == host_map->end())
		server_name = &host_map->begin()->second;
	else
		server_name = &host_map->find(client_ip)->second;
	setNonBlockingMode(new_sockfd);
	MOD_WRITE_READ(new_sockfd);
	_client_map.insert(std::make_pair(new_sockfd, new Client(TIMEOUT_SECS, server_name)));
	std::cout << "new connection -> " <<  GREEN << "client " << new_sockfd << RESET << std::endl;
}

#if (defined (LINUX) || defined (__linux__))
void ServerManager::eventManager() {
	struct epoll_event events[MAX_EVENTS];
	while (1) {
		int n_ready = epoll_wait(_epoll_fd, events, MAX_EVENTS, WAIT_TIMEOUT_SECS * 1000);
		if (n_ready == -1)
			throw std::runtime_error("epoll_wait");
		for (int i = 0; i < n_ready; i++) {
			int fd = events[i].data.fd;
			host_level2* serv = isPartOfListenFd(fd);
			if (serv)
				handleNewConnection(fd, serv);
			else {
				fd_client_pair client = *_client_map.find(fd);
				if (events[i].events & EPOLLIN)
					handleReadEvent(client);
				else if (events[i].events & EPOLLOUT)
					handleWriteEvent(client);
			}
		}
		timeoutCheck();
	}
}
#else
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
			if (serv)
				handleNewConnection(fd, serv);
			else {
				fd_client_pair client = *_client_map.find(fd);
				if (events[i].filter == EVFILT_READ)
					handleReadEvent(client);
				else if (events[i].filter == EVFILT_WRITE)
					handleWriteEvent(client);
			}
		}
		timeoutCheck();
	}
}
#endif

void ServerManager::handleReadEvent(fd_client_pair client)  {
	if (!readFromClient(client)) {
		client.second->StopTimer();
		client.second->CreateResponse();
		client.second->SetReadyToWrite(true);
	}
}

void ServerManager::handleWriteEvent(fd_client_pair client) {
	if (client.second->IsReadyToWrite())
	{
		writeToClient(client.first, client.second->GetResponseHeader());
		writeToClient(client.first, client.second->GetResponseBody());
		client.second->ResetRequestContext();
		connectionCloseMode(client);
	}
}

void ServerManager::closeClientConnection(fd_client_pair client) {
	DEL_EVENT(client.first);
	delete client.second;
	_client_map.erase(client.first);
	close(client.first);
	std::cout << "connection closed ->" << RED << " client " << client.first << RESET << std::endl;
}

void ServerManager::closeClientConnection(fd_client_pair client, map_iterator_type elem) {
	DEL_EVENT(client.first);
	delete client.second;
	_client_map.erase(elem);
	close(client.first);
	std::cout << "connection closed ->" << RED << " client " << client.first << RESET << std::endl;
}

void ServerManager::connectionCloseMode(fd_client_pair client) {
	if (!client.second->IsKeepAlive())
		closeClientConnection(client);
}

int	ServerManager::readFromClient(fd_client_pair client) {
	char buffer[BUFFER_SIZE + OVERLAP];
	const ssize_t nbytes = recv(client.first, buffer + OVERLAP, BUFFER_SIZE, 0);
	if (client.second->HasBodyExceeded()) // ignore incoming data
		return 1;
	if (nbytes <= 0) {
		closeClientConnection(client);
		return 1;
	}
	return (client.second->TreatReceivedData(buffer + OVERLAP, nbytes));
}

void ServerManager::writeToClient(int client_fd, const std::string &str) {
    size_t n = str.length();
    ssize_t nbytes = 0;
    const char *ptr = str.c_str();
    size_t chunkSize;
    while (n) {
        chunkSize = n > (size_t)BUFFER_SIZE ? BUFFER_SIZE : n;
        nbytes = send(client_fd, ptr, chunkSize, 0);
        if (nbytes == -1)
		    throw std::runtime_error("send()");
        n -= nbytes;
        ptr += nbytes;
    }
}

ServerManager::~ServerManager() {
	for (fd_port_level1::iterator it = _list_server_map.begin(); it != _list_server_map.end(); ++it) {
		std::cout << "connection closed ->" << RED << " server " << it->first << RESET << std::endl;
		close(it->first);
	}
	for (auto it = _client_map.begin(); it != _client_map.end(); ) {
        auto currentIt = it++;
		closeClientConnection(*currentIt);
    }
	CLOSE_EPOLL;
}



#if (defined (LINUX) || defined (__linux__))
# define INIT_EPOLL { \
	_epoll_fd = epoll_create1(0); \
	if (_epoll_fd == -1) { \
		throw std::runtime_error("epoll: create"); \
	} \
}
#else
# define INIT_EPOLL { \
	_kqueue_fd = kqueue(); \
	if (_kqueue_fd == -1) { \
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
# define MOD_READ(socket) { \
	struct epoll_event event; \
	event.data.fd = socket; \
	event.events = EPOLLIN; \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket, &event) == -1) { \
		throw std::runtime_error("epoll_ctl: add"); \
	} \
}
#else
# define MOD_READ(socket) { \
	struct kevent event; \
	EV_SET(&event, socket, EVFILT_READ, EV_ADD, 0, 0, NULL); \
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

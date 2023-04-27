

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
# define DEL_EVENT(socket) { \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, socket, NULL) == -1) { \
		throw std::runtime_error("epoll_ctl: delete"); \
	} \
}
#else
# define DEL_EVENT(socket) { \
	struct kevent events[2]; \
	EV_SET(events, socket, EVFILT_READ, EV_DELETE, 0, 0, 0); \
	EV_SET(events + 1, socket, EVFILT_WRITE, EV_DELETE, 0, 0, 0); \
	if (kevent(_kqueue_fd, events, 2, NULL, 0, NULL ) < 0) { \
		throw std::runtime_error("kevent delete"); \
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
# define MOD_WRITE(socket) { \
	struct epoll_event event; \
	event.data.fd = socket; \
	event.events = EPOLLOUT; \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket, &event) == -1) { \
		throw std::runtime_error("epoll_ctl: mod"); \
	} \
}
#else
# define MOD_WRITE(socket) { \
	struct kevent event; \
	EV_SET(&event, socket, EVFILT_WRITE, EV_ADD, 0, 0, NULL); \
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) == -1) { \
		throw std::runtime_error("kevent: add"); \
	} \
}
#endif


#if (defined (LINUX) || defined (__linux__))
# define MOD_WRITE_READ(socket) { \
	struct epoll_event event; \
	event.data.fd = socket; \
	event.events = EPOLLIN | EPOLLOUT; \
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket, &event) < 0) \
		throw std::runtime_error("epoll_ctl EPOLL_CTL_ADD"); \
}
#else
# define MOD_WRITE_READ(socket) { \
	struct kevent event; \
	EV_SET(&event, socket, EVFILT_READ, EV_ADD, 0, 0, NULL);\
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)\
		throw std::runtime_error("kevent add read");\
	EV_SET(&event, socket, EVFILT_WRITE, EV_ADD, 0, 0, NULL);\
	if (kevent(_kqueue_fd, &event, 1, NULL, 0, NULL) < 0)\
		throw std::runtime_error("kevent add write");\
}
#endif

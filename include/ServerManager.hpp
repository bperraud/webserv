#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Config.hpp"

class ServerManager {

private:


public:
	ServerManager(Config config);
	void pollSockets();
	void listenAll();
	void handleNewConnections();
	void handleRequests();
	~ServerManager();
};

#endif

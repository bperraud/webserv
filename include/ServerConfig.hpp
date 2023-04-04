#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "JsonParser.hpp"

#include <list>

enum e_routes {
	ROOT,
	INDEX,
	METHODS,
	AUTO_INDEX,
	CGI
};

struct routes {
	std::string root;
	std::string methods[3];
	std::map<std::string, std::string> cgi;
	bool		auto_index;
};

struct server {
	std::string host;
	int			PORT;
	int			max_body_size;
	std::map<std::string, routes> routes_map;
};

class ServerConfig {

private:
	std::list<server>	_server_list;

public:
    ServerConfig(const JsonParser &parser);
	server parseJsonObject(const json_value &json_object);

	std::list<server> getServerList() const;
    ~ServerConfig();
};

std::ostream& operator<<(std::ostream& os, const server& s);

#endif

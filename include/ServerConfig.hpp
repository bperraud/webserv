#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "JsonParser.hpp"

#include <vector>

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
	std::vector<server>	_server_vector;

public:
    ServerConfig(const std::vector<json_value> &json_vector);
	void parseJsonObject(const json_value &json_object);
    ~ServerConfig();
};

#endif

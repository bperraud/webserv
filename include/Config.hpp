#ifndef CONFIG_HPP
#define CONFIG_HPP


#include <map>
#include <string>
#include <iostream>
#include <fstream>

enum e_routes {
	ROOT,
	INDEX,
	METHODS,
	AUTO_INDEX,
	CGI
};

struct routes {
	std::string root;
	std::string index;
	std::string methods[3];
	std::string cgi;
	bool		auto_index;
};

class Config {

private:
	std::map<int, routes>	_routes;
	std::string				_server_name;
	int						_PORT;
	std::string				_host;
	int						_max_body_size;

public:
    Config(char *configFile);

	int parseFile(char *configFile);

    ~Config();
};

#endif

#ifndef CONFIG_HPP
#define CONFIG_HPP


#include <map>
#include <string>

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
}

class Config {

private:
	std::map<int, routes> _routes;
	std::string _server_name;

public:
    Config(char * configFile);
    ~Config();
};

#endif

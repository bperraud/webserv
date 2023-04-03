#ifndef CONFIG_HPP
#define CONFIG_HPP


#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream> 	//stringstream

enum e_routes {
	ROOT,
	INDEX,
	METHODS,
	AUTO_INDEX,
	CGI
};

// Define the possible types of JSON values
enum json_type {
    null,
    boolean,
    integer,
    string,
    array,
    object
};

// Define a struct to hold a JSON value
struct json_value {
    json_type 									type;
    bool 										boolean_value ;
    double 										number_value ;
    std::string									string_value;
    std::vector<json_value>						array_value;
    std::map<std::string, json_value> 			object_value;
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

	std::string trim(const std::string& str);
	json_value parse_value(std::string& str);
	json_value parse_array(std::string& str);
	json_value parse_object(std::string& str);;

    ~Config();
};

std::ostream &operator<<(std::ostream &os, const json_value obj);

#endif

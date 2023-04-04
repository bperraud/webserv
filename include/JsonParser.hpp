#ifndef JSONPARSER_HPP
#define JSONPARSER_HPP

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream> 	//stringstream

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

class JsonParser {

private:
	json_value 	_json_config;

public:
    JsonParser(char *JsonParserFile);

	int parseFile(char *JsonParserFile);

	std::string trim(const std::string& str);
	json_value parse_value(std::string& str);
	json_value parse_array(std::string& str);
	json_value parse_object(std::string& str);;

	json_value getJsonObject() const;

    ~JsonParser();
};

std::ostream &operator<<(std::ostream &os, const json_value obj);

#endif

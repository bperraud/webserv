#include "ServerConfig.hpp"

ServerConfig::ServerConfig(const json_value &json_object) {
	parseJsonObject(json_object);
}


void ServerConfig::parseJsonObject(const json_value &json_object) {
	std::map<std::string, json_value>::const_iterator it;
	for (it = json_object.object_value.begin(); it != json_object.object_value.end(); ++it) {
		std::cout << it->first << " => " << it->second << '\n';
	}
}

ServerConfig::~ServerConfig() {

}

#include "ServerConfig.hpp"

ServerConfig::ServerConfig(const std::vector<json_value> &_json_object_vector) {
	std::vector<json_value>::const_iterator it;
	for (it = _json_object_vector.begin(); it != _json_object_vector.end(); it++) {
		_server_vector.push_back(parseJsonObject(*it));
	}
}

server ServerConfig::parseJsonObject(const json_value &json_object) {
	server server_config;
	std::map<std::string, routes> routes_map;
	std::map<std::string, json_value>::const_iterator it;
	std::map<std::string, json_value>::const_iterator sub_it;

	for (it = json_object.object_value.begin(); it != json_object.object_value.end(); it++) {
		std::string key = it->first;
		const json_value& value = it->second;

		if (key == "listen") {
			if (value.array_value.size() >= 1) {
				server_config.host = value.array_value[0].string_value;
			}
			if (value.array_value.size() >= 2) {
				server_config.PORT = static_cast<int>(value.array_value[1].number_value);
			}
		} else if (key == "max_body_size") {
			server_config.max_body_size = static_cast<int>(value.number_value);
		} else if (key == "routes") {
			for (sub_it = value.object_value.begin(); sub_it != value.object_value.end(); sub_it++) {
				std::string route_key = sub_it->first;
				const json_value& route_value = sub_it->second;
				routes route;

				for (sub_it = route_value.object_value.begin(); sub_it != route_value.object_value.end(); sub_it++) {
					std::string route_info_key = sub_it->first;
					const json_value& route_info_value = sub_it->second;

					if (route_info_key == "root") {
						route.root = route_info_value.string_value;
					} else if (route_info_key == "methods") {
						for (size_t i = 0; i < route_info_value.array_value.size(); i++) {
							route.methods[i] = route_info_value.array_value[i].string_value;
						}
					} else if (route_info_key == "cgi") {
						for (sub_it = route_info_value.object_value.begin(); sub_it != route_info_value.object_value.end(); sub_it++) {
							std::string cgi_key = sub_it->first;
							const json_value& cgi_value = sub_it->second;
							route.cgi[cgi_key] = cgi_value.string_value;
						}
					} else if (route_info_key == "auto_index") {
						route.auto_index = route_info_value.boolean_value;
					}
				}
				routes_map[route_key] = route;
			}
			server_config.routes_map = routes_map;
		}
	}
	return server_config;
}


ServerConfig::~ServerConfig() {

}

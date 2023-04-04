#include "ServerConfig.hpp"

ServerConfig::ServerConfig(const JsonParser &parser) {
	std::vector<json_value>::const_iterator it;
	std::vector<json_value> json_object_vector = parser.getJsonVector();
	for (it = json_object_vector.begin(); it != json_object_vector.end(); it++) {
		server s = parseJsonObject(*it);
		_server_list.push_back(s);
		std::cout << "server : " << s << std::endl;
	}
}

ServerConfig::~ServerConfig() {

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
            server_config.PORT = value.array_value[1].number_value;
        }
		} else if (key == "max_body_size") {
			server_config.max_body_size = value.number_value;
		} else if (key == "routes") {
			for (sub_it = value.object_value.begin(); sub_it != value.object_value.end(); sub_it++) {
				std::string route_key = sub_it->first;
				const json_value& route_value = sub_it->second;
				routes route;

				std::map<std::string, json_value>::const_iterator sub_sub_it;
				for (sub_sub_it = route_value.object_value.begin(); sub_sub_it != route_value.object_value.end(); sub_sub_it++) {
					std::string route_info_key = sub_sub_it->first;
					const json_value& route_info_value = sub_sub_it->second;

					if (route_info_key == "root") {
						route.root = route_info_value.string_value;
					} else if (route_info_key == "methods") {
						for (size_t i = 0; i < route_info_value.array_value.size(); i++) {
							route.methods[i] = route_info_value.array_value[i].string_value;
						}
					} else if (route_info_key == "cgi") {
						for (std::map<std::string, json_value>::const_iterator sub_sub_sub_it = route_info_value.object_value.begin(); sub_sub_sub_it != route_info_value.object_value.end(); sub_sub_sub_it++) {
							std::string cgi_key = sub_sub_sub_it->first;
							const json_value& cgi_value = sub_sub_sub_it->second;
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

std::ostream& operator<<(std::ostream& os, const server& s) {
    os << "host: " << s.host << std::endl;
    os << "port: " << s.PORT << std::endl;
    os << "max body size: " << s.max_body_size << std::endl;
    os << "routes: " << std::endl;
    std::map<std::string, routes>::const_iterator routes_it;
    std::map<std::string, std::string>::const_iterator cgi_it;
    for (routes_it = s.routes_map.begin(); routes_it != s.routes_map.end(); ++routes_it) {
        os << "    route: " << routes_it->first << std::endl;
        const routes& r = routes_it->second;
        os << "        root: " << r.root << std::endl;
        os << "        methods: ";
        for (int i = 0; i < 3; i++) {
            if (!r.methods[i].empty()) {
                os << r.methods[i] << " ";
            }
        }
        os << std::endl;
        os << "        CGI: " << std::endl;
        for (cgi_it = r.cgi.begin(); cgi_it != r.cgi.end(); ++cgi_it) {
            os << "            " << cgi_it->first << ": " << cgi_it->second << std::endl;
        }
        os << "        auto index: " << std::boolalpha << r.auto_index << std::endl;
    }
    return os;
}

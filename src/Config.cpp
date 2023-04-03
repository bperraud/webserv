#include "Config.hpp"


Config::Config(char *configFile) {
	parseFile(configFile);
}

int Config::parseFile(char *configFile) {

	std::ifstream infile(configFile);

	if (!infile.is_open()) { // check if the file was opened successfully
		std::cout << "Unable to open file" << std::endl; // handle the error
		return 1;
	}
		std::string line;
		while (std::getline(infile, line)) { // read a line at a time from the file
		std::cout << line << std::endl; // print the line to the console
	}
	infile.close();

	return 0;
}

Config::~Config() {

}

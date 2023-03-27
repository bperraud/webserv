#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h> // stat()
#include <dirent.h> //closedir()

class Utils {

private:


public:
    Utils();

	static bool 		pathToFileExist(const std::string& path);
	static bool 		isDirectory(const std::string& path);
	static std::string	intToString(int value);

    ~Utils();
};

#endif

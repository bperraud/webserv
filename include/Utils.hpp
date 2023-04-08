#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h> // stat()
#include <dirent.h> //closedir()
#include <iostream>
#include <unistd.h>
#include <iomanip> // setfill()
#include <dirent.h>	// opendir()

class Utils {

private:


public:
    Utils();

	static bool 		pathToFileExist(const std::string& path);
	static bool 		isDirectory(const std::string& path);
	static std::string	intToString(int value);

	static std::string 	urlDecode(const std::string &url);
	static void			loadFile(const std::string &fileName, std::basic_iostream<char> &stream) ;

	static std::ofstream*	createOrEraseFile(std::string fileName);
	static int				directoryListing(const std::string &dir);
	static bool				hasExecutePermissions(const char* filepath);
    ~Utils();
};

#endif

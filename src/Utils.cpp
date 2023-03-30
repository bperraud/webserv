#include "Utils.hpp"

Utils::Utils() {

}

bool Utils::pathToFileExist(const std::string& path) {
	std::ifstream file(path.c_str());
    return (file.is_open());
}

bool Utils::isDirectory(const std::string& path) {
	struct stat filestat;
    if (stat(path.c_str(), &filestat) != 0)
    {
        return false;
    }
    return S_ISDIR(filestat.st_mode);
}

bool Utils::hasExecutePermissions(const char* filepath) {
	return access(filepath, X_OK) == 0;
}

std::string Utils::intToString(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

void Utils::loadFile(const std::string &fileName, std::basic_iostream<char> &stream) {
	std::ifstream input_file(fileName.c_str());
	stream << input_file.rdbuf();
}

std::ofstream* Utils::createOrEraseFile(const char *filename) {
    // Check if file exists
	if (remove(filename) != 0) {
        // File does not exist, so proceed with creating it
        std::ofstream* file = new std::ofstream(filename);
        if (!(*file)) {
            delete file;
            return NULL; // Return null pointer on error
        }
        return file;
    } else {
        // File exists and has been erased, so create it again
        std::ofstream* file = new std::ofstream(filename);
        if (!(*file)) {
            delete file;
            return NULL; // Return null pointer on error
        }
        return file;
    }
}


Utils::~Utils() {

}

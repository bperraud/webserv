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

std::string Utils::intToString(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

Utils::~Utils() {

}

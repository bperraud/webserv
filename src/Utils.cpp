#include "Utils.hpp"

namespace Utils {

bool pathToFileExist(const std::string& path) {
	std::cout << " path " << path << std::endl;

	std::ifstream file(path.c_str());
	return (file.is_open());
}

bool isDirectory(const std::string& path) {
	struct stat filestat;
	if (stat(path.c_str(), &filestat) != 0)
	{
		return false;
	}
	return S_ISDIR(filestat.st_mode);
}

bool hasExecutePermissions(const char* filepath) {
	return access(filepath, X_OK) == 0;
}

int directoryListing(const std::string &directory_path) {
	DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(directory_path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::cout << ent->d_name << std::endl;
        }
        closedir (dir);
    } else {
        std::cerr << "Could not open directory." << std::endl;
        return 1;
    }
    return 0;
}

std::string intToString(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

void loadFile(const std::string &fileName, std::basic_iostream<char> &stream) {
	std::ifstream input_file(fileName.c_str());
	stream << input_file.rdbuf();
}

std::ofstream* createOrEraseFile(std::string fileName) {
	// Check if file exists
	const char* filename = fileName.c_str();
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

std::string urlDecode(const std::string& encoded_url) {
	std::stringstream decoded_url;
	decoded_url << std::hex << std::setfill('0');

	for (size_t i = 0; i < encoded_url.length(); ++i) {
		if (encoded_url[i] == '%') {
			if (i + 2 < encoded_url.length()) {
				int hex_value = 0;
				std::istringstream hex_str(encoded_url.substr(i + 1, 2));
				hex_str >> std::hex >> hex_value;
				decoded_url << static_cast<char>(hex_value);
				i += 2;
			}
		}
		else if (encoded_url[i] == '+') {
			decoded_url << ' ';
		}
		else {
			decoded_url << encoded_url[i];
		}
	}
	return decoded_url.str();
}

}; // namespace Utils

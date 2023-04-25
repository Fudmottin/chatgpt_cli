#include "utils.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <pwd.h>

using namespace std;

string read_api_key_from_file(const string& filename) {
    ifstream file(filename);
    if (!file) {
        throw runtime_error("Failed to open file: " + filename);
    }

    string line;
    while (getline(file, line)) {
        // Skip lines that begin with a comment character.
        if (line[0] == '#') {
            continue;
        }
        istringstream iss(line);
        string key, value;
        if (getline(iss, key, '=') && getline(iss, value)) {
            if (key == "OPENAI_API_KEY") {
                return value;
            }
        }
    }
    throw runtime_error("API key not found in file: " + filename);
}

string get_home_directory() {
    const char* home_dir = getenv("HOME");

    if (!home_dir) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            home_dir = pwd->pw_dir;
        }
    }

    return home_dir ? string(home_dir) : "";
}


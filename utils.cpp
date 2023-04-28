#include "utils.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
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

string get_api_key() {
    const char* api_key_env_var = getenv("OPENAI_API_KEY");
    string api_key;

    if (api_key_env_var != nullptr)
        api_key = api_key_env_var;
    else
        api_key = read_api_key_from_file(get_home_directory() + "/.env");

    if (api_key.empty()) {
        cerr << "Failed to read the API key from the .env file. Please ensure it is properly set." << endl;
	return "";
    }

    return api_key;
}

string get_chatgpt_cli_dir() {
    string home_dir = get_home_directory();
    string chatgpt_cli_dir = home_dir.empty() ? "" : home_dir + "/.chatgpt_cli";
    if (!chatgpt_cli_dir.empty()) {
        struct stat st;
        if (stat(chatgpt_cli_dir.c_str(), &st) == -1) {
            if (mkdir(chatgpt_cli_dir.c_str(), 0700) == -1) {
                // Error: could not create directory
                chatgpt_cli_dir = "";
            }
        } else if (!S_ISDIR(st.st_mode)) {
            // Error: path exists but is not a directory
            chatgpt_cli_dir = "";
        }
    }
    return chatgpt_cli_dir;
}


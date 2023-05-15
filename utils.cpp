#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <ctime>
#include <iomanip>
#include <pwd.h>
#include <histedit.h>
#include "utils.h"

namespace util {
    std::string history_filename;
}

using namespace std;

namespace util {

float clamp(float val, float lower, float upper) {
    return min(max(val, lower), upper);
}

void save_history_to_file(History *hist) {
    ofstream history_file(util::history_filename, ios::out);

    if (!history_file.is_open()) {
        cerr << "Failed to open history file for writing." << endl;
        return;
    }

    HistEvent ev;
    history(hist, &ev, H_FIRST);
    do {
        history_file << ev.str;
    } while (history(hist, &ev, H_NEXT) == 0);

    history_file.close();
}

void load_history_from_file(History *hist) {
    ifstream history_file(util::history_filename);

    if (!history_file.is_open()) {
        return;
    }

    HistEvent ev;
    string line;
    vector<string> lines;
    while (getline(history_file, line)) {
        lines.push_back(line);
    }

    reverse(lines.begin(), lines.end());
    for (const auto& line : lines) {
        history(hist, &ev, H_ENTER, line.c_str());
    }

    history_file.close();
}

string remove_quotes(const string &input) {
    string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '\\') {
            if (i + 1 < input.size() && input[i + 1] == '"') {
                // Escaped quote, ignore both characters
                i++;
            } else {
                // Keep the backslash character
                result.push_back(input[i]);
            }
        } else if (input[i] != '"') {
            // Keep the non-quote character
            result.push_back(input[i]);
        }
    }

    return result;
}

string trim_content(const string& content, size_t max_words) {
    string modified_content = content;

    // Remove "user: " and "AI: " prefixes
    if (modified_content.substr(0, 6) == "user: ") {
        modified_content = modified_content.substr(6);
    } else if (modified_content.substr(0, 4) == "AI: ") {
        modified_content = modified_content.substr(4);
    }

    istringstream iss(modified_content);
    string word;
    string result;
    size_t word_count = 0;

    while (iss >> word && word_count < max_words) {
        if (word_count > 0) {
            result += " ";
        }
        result += word;
        word_count++;
    }

    if (iss >> word) { // Check if there are more words in the original content
        result += "...";
    }

    return result;
}

string get_formatted_time() {
    time_t t = time(nullptr);
    tm tm = *localtime(&t);

    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d-%H:%M:%S");

    return oss.str();
}

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

} // namespace util

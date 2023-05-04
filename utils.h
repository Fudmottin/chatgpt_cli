#pragma once

#include <editline/readline.h>
#include <histedit.h>

using namespace std;

namespace util {
float clamp(float val, float lower, float upper);
string read_api_key_from_file(const string& file_path);
string get_home_directory();
string get_api_key();
string get_chatgpt_cli_dir();
string get_formatted_time();
string trim_content(const string& content, size_t max_length);
void save_history_to_file(History *hist);
void load_history_from_file(History *hist);

extern string history_filename;
}

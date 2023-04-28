#pragma once

#include <string>
#include <iostream>

using namespace std;

string read_api_key_from_file(const string& file_path);
string get_home_directory();
string get_api_key();
string get_chatgpt_cli_dir();

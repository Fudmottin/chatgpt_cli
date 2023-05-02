#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <histedit.h>
#include "utils.h"
#include <algorithm>
#include "chatgpt_client.h"

using namespace std;
using CommandHandler = void(*)(AIClient&, const vector<string>&);

// Hidious global varialbes
static map<string, CommandHandler> command_map;
static EditLine *el = 0;
static History *hist = 0;

void quit_command(AIClient& ai_client, const vector<string>& parts) {
    cout << "Quitting program." << endl;
    string chatgpt_cli_dir = util::get_chatgpt_cli_dir();
    if (chatgpt_cli_dir != "") {
        string time_stamp = util::get_formatted_time();
	string history_file = chatgpt_cli_dir + "/chatgpt_history_" +
		time_stamp + ".json";
	if (ai_client.save_history(history_file)) cout << "History saved." << endl;
    }
    // Clear the input buffer before exiting
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    exit(0);
}

void set_chatgpt_temperature(AIClient& ai_client, const vector<string>& parts) {
    ChatGPTClient* chatgpt_client = dynamic_cast<ChatGPTClient*>(&ai_client);
    if (chatgpt_client) {
	try {
		float argument_value = stof(parts[1]);
		chatgpt_client->set_temperature(argument_value);
	} catch (const invalid_argument& e) {
		cerr << "Invalid argument: " << e.what() << endl;
	} catch (const out_of_range& e) {
		cerr << "Out of range: " << e.what() << endl;
	}
    } else {
	cout << "This command is only supported for ChatGPTClient objects." << endl;
    }
}

// Add more command functions here

void register_commands() {
    command_map["quit"] = quit_command;
    command_map["exit"] = quit_command;
    command_map["set-chatgpt-temperature"] = set_chatgpt_temperature;
    // Register more commands here
}

void handle_command(const string& command, AIClient& ai_client) {
    // Remove leading and trailing whitespace
    string trimmed_command = command;
    size_t start_pos = command.find_first_not_of(" \t\n\r\f\v");
    if (start_pos != string::npos) {
        size_t end_pos = command.find_last_not_of(" \t\n\r\f\v");
        trimmed_command = command.substr(start_pos, end_pos - start_pos + 1);
    }

    // Convert the command to lowercase for case-insensitivity
    transform(trimmed_command.begin(), trimmed_command.end(), trimmed_command.begin(),
        [](unsigned char c){ return tolower(c); });

    // Remove leading "/" character if it exists
    if (trimmed_command[0] == '/') {
        trimmed_command = trimmed_command.substr(1);
    }

    // Split the command into parts using whitespace as a delimiter
    vector<string> parts;
    istringstream iss(trimmed_command);
    string part;
    while (iss >> part) {
        parts.push_back(part);
    }

    if (!parts.empty()) {
        auto cmd_handler = command_map.find(parts[0]);
        if (cmd_handler != command_map.end()) {
            cmd_handler->second(ai_client, parts);
        } else {
            // Handle other commands or invalid commands
        }
    }
}

void cleanup() {
    util::save_history_to_file(hist);
    history_end(hist);
    el_end(el);
}

// Prompt function
const char *prompt(EditLine *e) {
    return "> ";
}

int main(int argc, char *argv[]) {
    register_commands();
    string api_key = util::get_api_key();

    util::history_filename = util::get_chatgpt_cli_dir() + "/history";

    ChatGPTClient chatgpt(api_key, "https://api.openai.com/v1/chat/completions");

    // Register the cleanup function to be called when the program exits
    atexit(cleanup);


    // Initialize the EditLine and History objects
    el = el_init(argv[0], stdin, stdout, stderr);
    if (!el) {
        cerr << "Failed to initialize EditLine." << endl;
        return 1;
    }

    hist = history_init();
    if (!hist) {
        cerr << "Failed to initialize History." << endl;
        el_end(el);
        return 1;
    }

    // Configure the EditLine and History objects
    el_set(el, EL_PROMPT, prompt);
    el_set(el, EL_EDITOR, "emacs");
    el_set(el, EL_HIST, history, hist);
    el_set(el, EL_SIGNAL, 1);
    el_set(el, EL_BIND, "^[[A", "ed-prev-history", NULL);
    el_set(el, EL_BIND, "^[[B", "ed-next-history", NULL);

    HistEvent ev;
    history(hist, &ev, H_SETSIZE, 500);
    util::load_history_from_file(hist);

    // Main loop
    int count;
    const char *line;
    string multi_line_input;
    while ((line = el_gets(el, &count)) != nullptr) {
	try {
    	    if (count > 1) {
	        if (line[count - 2] == '\\') {
		    multi_line_input.append(line, count);
	        } else {
		    multi_line_input.append(line);
		    if (!multi_line_input.empty() && multi_line_input.front() == '/') {
			handle_command(multi_line_input, chatgpt);
			history(hist, &ev, H_ENTER, multi_line_input.c_str());
		    } else {
		        history(hist, &ev, H_ENTER, multi_line_input.c_str());
			chatgpt.send_message(multi_line_input);
			cout << chatgpt.get_response();
		    }
		    multi_line_input.clear();
		}
	     }
	}
	catch (...) {
	    cout << "Unhandled exception. Sorry. Quitting as gracefully as I can.\n";
            break;
	}
    }

    return 0;
}

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
#include "chatgptapp.h"
#include "chatgpt_client.h"

using namespace std;
using CommandHandler = void(*)(AIClient&, const vector<string>&);

std::map<std::string, CommandHandler> command_map;

void quit_command(AIClient& ai_client, const vector<string>& parts) {
    cout << "Quitting program." << endl;
    string chatgpt_cli_dir = get_chatgpt_cli_dir();
    if (chatgpt_cli_dir != "") {
        string time_stamp = get_formatted_time();
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
		float argument_value = std::stof(parts[1]);
		chatgpt_client->set_temperature(argument_value);
	} catch (const std::invalid_argument& e) {
		std::cerr << "Invalid argument: " << e.what() << std::endl;
	} catch (const std::out_of_range& e) {
		std::cerr << "Out of range: " << e.what() << std::endl;
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

// Prompt function
const char *prompt(EditLine *e) {
    return "> ";
}

int main(int argc, char *argv[]) {
    register_commands();
    string api_key = get_api_key();
    util::history_filename = get_chatgpt_cli_dir() + "/history";

    ChatGPTClient chatgpt(api_key, "https://api.openai.com/v1/chat/completions");

    // Initialize the EditLine and History objects
    EditLine *el = el_init(argv[0], stdin, stdout, stderr);
    if (!el) {
        cerr << "Failed to initialize EditLine." << endl;
        return 1;
    }

    History *hist = history_init();
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
    load_history_from_file(hist);

    ChatGPTApp app(el, hist);

    // Main loop
    int count;
    const char *line;
    string multi_line_input;
    while ((line = el_gets(el, &count)) != nullptr) {
        if (count > 1) {
            // If the line ends with a backslash, store it and continue
            if (line[count - 2] == '\\') {
                multi_line_input.append(line, count);
            } else {
                // Combine the stored input with the current line
                multi_line_input.append(line);

                // Check if the input is a command
                if (!multi_line_input.empty() && multi_line_input.front() == '/') {
                    handle_command(multi_line_input, chatgpt);
                    history(hist, &ev, H_ENTER, multi_line_input.c_str());
                } else {
                    history(hist, &ev, H_ENTER, multi_line_input.c_str());
                    chatgpt.send_message(multi_line_input);
		    cout << chatgpt.get_response();
                }

                // Reset the stored input for the next command
                multi_line_input.clear();
            }
        }
    }

    return 0;
}

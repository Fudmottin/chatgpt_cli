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
#include "chat_client.h"
#include "openai_image.h"

using namespace std;
using CommandHandler = void(*)(OpenAIClient&, const vector<string>&);

void handle_command(const string& command, OpenAIClient& ai_client);

// Hidious global varialbes
static map<string, CommandHandler> command_map;
static EditLine *el = 0;
static History *hist = 0;

void quit_command(OpenAIClient& ai_client, const vector<string>& parts) {
    cout << "Quitting program." << endl;
    string chatgpt_cli_dir = util::get_chatgpt_cli_dir();
    if (chatgpt_cli_dir != "") {
        string time_stamp = util::get_formatted_time();
	string history_file = chatgpt_cli_dir + "/chatgpt_history_" +
		time_stamp + ".txt";
	if (ai_client.save_history(history_file)) cout << "History saved." << endl;
    }
    // Clear the input buffer before exiting
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    exit(0);
}

void set_temperature(OpenAIClient& ai_client, const vector<string>& parts) {
    try {
        float argument_value = stof(parts[1]);
        ai_client.set_temperature(argument_value);
    } catch (const std::invalid_argument& e) {
        cout << "Invalid temperature value. Please provide a valid number." << endl;
    } catch (const std::out_of_range& e) {
        cout << "Temperature value out of range. Please provide a valid number within the acceptable range." << endl;
    }
}

void make_image_command(OpenAIClient& ai_client, const vector<string>& parts) {
    if (parts.size() < 2) {
        cout << "Usage: /make-image image prompt" << endl;
        return;
    }

    string image_prompt;
    for (size_t i = 1; i < parts.size(); ++i) {
        image_prompt += parts[i];
        if (i < parts.size() - 1) {
            image_prompt += " ";
        }
    }

    OpenAIImage image_generator(ai_client.get_api_key());
    string filenames = image_generator.send_message(image_prompt);

    if (!filenames.empty()) {
        cout << "\nGenerated images saved at: " << endl << filenames << endl;
    } else {
        cout << "Failed to generate images." << endl;
    }
}

void prompt_image_command(OpenAIClient& ai_client, const vector<string>& parts) {
    if (parts.size() < 2) {
        cout << "Usage: /prompt-image image prompt" << endl;
        return;
    }

    string image_prompt;
    for (size_t i = 1; i < parts.size(); ++i) {
        image_prompt += parts[i];
        if (i < parts.size() - 1) {
            image_prompt += " ";
        }
    }

    image_prompt += " Create a prompt for DALL-E that is fewer than 1000 characters.";

    string input = ai_client.send_message(image_prompt);
    size_t colonPos = input.find(':');

    if (colonPos != std::string::npos) {
        string trimmed = input.substr(colonPos + 1);
        trimmed.erase(0, trimmed.find_first_not_of(" "));
        input = trimmed;
    }

    input = util::remove_quotes(input);

    cout << "\nCalling /make-image " + input << endl;

    string new_prompt = "/make-image " + input;
    handle_command(new_prompt, ai_client);
}

// Add more command functions here

void register_commands() {
    command_map["quit"] = quit_command;
    command_map["exit"] = quit_command;
    command_map["set-chatgpt-temperature"] = set_temperature;
    command_map["make-image"] = make_image_command;
    command_map["prompt-image"] = prompt_image_command;
    // Register more commands here
}

void handle_command(const string& command, OpenAIClient& ai_client) {
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
	    cout << "Command: " << parts[0] << " not recognized. Ignored.\n";
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

    ChatClient chatgpt(api_key);

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
		    multi_line_input.append(line, count -2);
	        } else {
		    multi_line_input.append(line);
		    if (!multi_line_input.empty() && multi_line_input.front() == '/') {
			handle_command(multi_line_input, chatgpt);
			history(hist, &ev, H_ENTER, multi_line_input.c_str());
		    } else {
		        history(hist, &ev, H_ENTER, multi_line_input.c_str());
			cout << chatgpt.send_message(multi_line_input) << endl;
		    }
		    multi_line_input.clear();
		}
	     }
	}
	catch (const runtime_error& e) { 
            cerr << "Caught runtime  error: " << e.what() << endl;
	    multi_line_input.clear();
	    continue;
        }
	catch (const exception& e) {
            cerr << "Caught an error: " << e.what() << endl;
	    multi_line_input.clear();
	    continue;
	}
	catch (...) {
	    cerr << "Unknown exception. Sorry. Quitting as gracefully as I can.\n";
            break;
	}
    }

    return 0;
}

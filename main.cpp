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

void handle_command(const string& command, ChatGPTClient& chatgpt);

void cleanup() {
    // Perform any necessary cleanup here, such as destroying the ChatGPTClient object
    // ...
}

// Prompt function
const char *prompt(EditLine *e) {
    return "> ";
}

int main(int argc, char *argv[]) {
    string api_key = get_api_key();

    ChatGPTClient chatgpt(api_key, "https://api.openai.com/v1/chat/completions");

    // Register the cleanup function to be called when the program exits
    atexit(cleanup);


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

    // Main loop
    int count;
    const char *line;
    string multi_line_input;
    while ((line = el_gets(el, &count)) != nullptr) {
        if (count > 1) {
            // If the line ends with a backslash, store it and continue
            if (line[count - 2] == '\\') {
                multi_line_input.append(line, count - 2); // Exclude the backslash and newline
            } else {
                // Combine the stored input with the current line
                multi_line_input.append(line);

                // Check if the input is a command
                if (!multi_line_input.empty() && multi_line_input.front() == '/') {
                    // Handle the command
                    handle_command(multi_line_input, chatgpt);
                } else {
                    // Add the multi-line input to the history and send it to the server
                    history(hist, &ev, H_ENTER, multi_line_input.c_str());
                    chatgpt.send_message(multi_line_input);
                }

                // Reset the stored input for the next command
                multi_line_input.clear();
            }
        }
    }

    // Cleanup
    history_end(hist);
    el_end(el);

    return 0;
}

void handle_command(const string& command, ChatGPTClient& chatgpt) {
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

    if (parts.empty()) {
        // Empty command, do nothing
    } else if (parts[0] == "quit") {
        cout << "Quitting program." << endl;

        // Clear the input buffer before exiting
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        exit(0);
    } else {
        // Handle other commands
    }
}


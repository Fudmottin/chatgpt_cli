#include <iostream>
#include <histedit.h>
#include <vector>
#include <cstring>
#include <histedit.h>
#include <cstring>
#include "utils.h"
#include "chatgpt_client.h"


// Prompt function
const char *prompt(EditLine *e) {
    return "> ";
}

int main(int argc, char *argv[]) {
    const char* api_key_env_var = std::getenv("OPENAI_API_KEY");
    string api_key;

    if (api_key_env_var != nullptr)
        api_key = api_key_env_var;
    else
        api_key = read_api_key_from_file(get_home_directory() + "/.env");

    if (api_key.empty()) {
        cerr << "Failed to read the API key from the .env file. Please ensure it is properly set." << endl;
        return 1;
    }

    ChatGPTClient chatgpt(api_key, "https://api.openai.com/v1/chat/completions");


    // Initialize the EditLine and History objects
    EditLine *el = el_init(argv[0], stdin, stdout, stderr);
    if (!el) {
        std::cerr << "Failed to initialize EditLine." << std::endl;
        return 1;
    }

    History *hist = history_init();
    if (!hist) {
        std::cerr << "Failed to initialize History." << std::endl;
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

                // Add the multi-line input to the history and print it
                history(hist, &ev, H_ENTER, multi_line_input.c_str());
                chatgpt.send_message(multi_line_input);

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


#include <iostream>
#include <cstdlib>
#include <string>
#include "chatgpt_client.h"
#include "utils.h"

using namespace std;

int main() {
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

    chatgpt.run_chat();

    return 0;
}


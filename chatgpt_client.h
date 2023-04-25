#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <ncurses.h>

using namespace std;
using json = nlohmann::json;

class ChatGPTClient {
public:
    ChatGPTClient(const string& api_key, const string& api_base_url);
    void run_chat();

private:
    json send_request(const json& request_data);
    string extract_response(const json& response);

    string api_key;
    string api_base_url;
    cpr::Session client;
    json conversation_history;
};


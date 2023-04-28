#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "utils.h"

using namespace std;
using json = nlohmann::json;

const vector<string> supported_models = {
	"gpt-3.5-turbo",
};

class ChatGPTClient {
public:
    ChatGPTClient(const string& api_key, const string& api_base_url);
    bool save_history(const string& file_name);
    void send_message(const string& message);
    void set_model(const string& model);
    void set_temperature(float temp);
    void set_topp(float topp);

private:
    json send_request(const json& request_data);
    string extract_response(const json& response);

    string api_key;
    string api_base_url;
    string model;
    float temperature;
    float topp;
    cpr::Session client;
    json conversation_history;
};


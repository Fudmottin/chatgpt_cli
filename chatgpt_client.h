#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "utils.h"
#include "ai_client.h"

using namespace std;
using json = nlohmann::json;

const vector<string> supported_models = {
	"gpt-3.5-turbo",
	"gpt-4"
};

class ChatGPTClient : public AIClient {
public:
    explicit ChatGPTClient(const string& api_key, const string& api_base_url);
    bool save_history(const string& file_name) override;
    string send_message(const string& message) override;

    void set_model(const string& model);
    void set_temperature(float temp);
    void set_presence_penalty(float penalty);
    void set_frequency_penalty(float penalty);
    void set_max_tokens(int tok);

private:
    json send_request(const json& request_data);
    string extract_response(const json& response);

    string model;
    string chatgpt_response;
    float temperature;
    float presence_penalty;
    float frequency_penalty;
    int max_tokens;
    int counter;
    json conversation_history;
};

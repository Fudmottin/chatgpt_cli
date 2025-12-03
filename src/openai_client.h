// chatgpt_cli openai_client.h

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "utils.h"

using namespace std;
using json = nlohmann::json;

class OpenAIClient {
public:
    OpenAIClient(const string& api_key);
    virtual ~OpenAIClient() = default;
    virtual string send_message(const string& message) = 0;
    virtual bool save_history(const string& file_name) = 0;

    void set_model(const string& model);
    void set_temperature(float temp);
    void set_presence_penalty(float penalty);
    void set_frequency_penalty(float penalty);
    void set_max_tokens(int tok);

    string get_api_key() const;

protected:
    cpr::Session client;
    string api_key;
    string api_base_url;
    string model;
    float temperature;
    float presence_penalty;
    float frequency_penalty;
    int max_tokens;
    vector<string> conversation_history;
};


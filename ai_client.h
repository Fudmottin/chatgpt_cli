#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using namespace std;
using json = nlohmann::json;

class AIClient {
public:
    AIClient(const string& api_key, const string& api_base_url);
    virtual ~AIClient() = default;
    virtual string send_message(const string& message) = 0;
    virtual bool save_history(const string& file_name) = 0;

protected:
    cpr::Session client;
    string api_key;
    string api_base_url;
};

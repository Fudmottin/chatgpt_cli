#pragma once

#include <string>

using namespace std;

class AIClient {
public:
    AIClient() = default;
    virtual ~AIClient() = default;
    virtual void send_message(const string& message) = 0;
    virtual string get_response() = 0;
    virtual bool save_history(const string& file_name) = 0;
};


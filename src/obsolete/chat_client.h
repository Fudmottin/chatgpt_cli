// clanker chat_client.h

#pragma once

#include "openai_client.h"

using namespace std;
using json = nlohmann::json;

const vector<string> supported_models = {"gpt-3.5-turbo", "gpt-4"};

class ChatClient : public OpenAIClient {
 public:
   explicit ChatClient(const string& api_key);
   bool save_history(const string& file_name) override;
   string send_message(const string& message) override;

 private:
   json send_request(const json& request_data);
   string extract_response(const json& response);
};


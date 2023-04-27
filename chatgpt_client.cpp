#include "chatgpt_client.h"
#include <cpr/cpr.h>
#include <iostream>

using namespace std;
using json = nlohmann::json;

ChatGPTClient::ChatGPTClient(const string& api_key, const string& api_base_url)
    : api_key(api_key), api_base_url(api_base_url), client() {
    conversation_history = json::array();
    client.SetUrl(cpr::Url{api_base_url});
    client.SetHeader(cpr::Header{
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"},
    });
}

void ChatGPTClient::send_message(const string& message) {
//      conversation_history.push_back({{"role", "system"}, {"content", "start chat"}});
//      conversation_history.push_back({{"role", "user"}, {"content", message}});

        json request_data = {
	    {"model", "gpt-3.5-turbo"},
            {"messages", json::array({
                {
                    {"role", "user"},
                    {"content", message}
                }
            })},
            {"temperature", 0.7}
    	};

        json response = send_request(request_data);
        string chatgpt_response = extract_response(response);

        cout << "Assistant: " << chatgpt_response << endl;

//      conversation_history.push_back({{"role", "assistant"}, {"content", chatgpt_response}});
}

json ChatGPTClient::send_request(const json& request_data) {
    auto body = request_data.dump();
    client.SetBody(cpr::Body{body});

    auto response = client.Post();

    return json::parse(response.text);
}

string ChatGPTClient::extract_response(const json& response) {
    if (response.contains("choices") && !response["choices"].empty()) {
        const auto& message = response["choices"][0]["message"]["content"].get<string>();
        return message;
    } else {
        if (response.contains("error")) {
            string error_msg = response["error"]["message"].get<string>();
            return "Error: " + error_msg;
        } else {
            return "Error: Unable to extract response.";
        }
    }
}

#include "chatgpt_client.h"

using namespace std;
using json = nlohmann::json;

ChatGPTClient::ChatGPTClient(const string& api_key, const string& api_base_url)
    : AIClient(api_key, api_base_url),
      model(supported_models[1]), temperature(0.7),
      chatgpt_response(), max_tokens(0), counter(0),
      presence_penalty(0.0), frequency_penalty(0.0) {
    conversation_history = json::array();
    conversation_history.push_back({{"role", "system"}, {"content", "start chat"}});
}

void ChatGPTClient::set_model(const string& model) {this->model = model;}

void ChatGPTClient::set_temperature(float temp) {temperature = temp;}

void ChatGPTClient::set_max_tokens(int tok) {max_tokens = tok;}

void ChatGPTClient::set_presence_penalty(float penalty) {presence_penalty = penalty;}

void ChatGPTClient::set_frequency_penalty(float penalty) {frequency_penalty = penalty;}

string ChatGPTClient::send_message(const string& message) {
        json request_data = {
	    {"model", model},
            {"messages", json::array({
                {
                    {"role", "user"},
                    {"content", message}
                }
            })},
            {"temperature", temperature},
    	};

	if (max_tokens != 0) request_data.emplace("max_tokens", max_tokens);
	if (presence_penalty != 0.0)
	    request_data.emplace("presence_penalty", presence_penalty);
	if (frequency_penalty != 0.0)
	    request_data.emplace("frequency_penalty", frequency_penalty);

        json response = send_request(request_data);
        chatgpt_response = extract_response(response);
	return string("\nAssitent: " + chatgpt_response + "\n");
}

json ChatGPTClient::send_request(const json& request_data) {
    const size_t max_history_length = 5;
    json context_history;

    if (conversation_history.size() > max_history_length) {
        context_history = json(conversation_history.end() - max_history_length, conversation_history.end());
    } else {
        context_history = conversation_history;
    }

    context_history.push_back(request_data["messages"].back());

    for (size_t i = 0; i < context_history.size() - 1; ++i) {
        string message_content = context_history[i]["content"].get<std::string>();
        string trimmed_content = util::trim_content(message_content, 100);
        context_history[i]["content"] = trimmed_content;
    }

    json updated_request_data = request_data;
    updated_request_data["messages"] = context_history;
    auto body = updated_request_data.dump();

    client.SetBody(cpr::Body{body});

    auto response = client.Post();

    if (response.status_code != 200) {
        cerr << "Error: " << response.status_code << " " << response.error.message << endl;
        throw runtime_error("Request failed");
    }

    if (response.text.empty()) {
        cerr << "Error: Empty response received" << endl;
        throw runtime_error("Empty response");
    }

    json response_json = json::parse(response.text);

    conversation_history.push_back(request_data["messages"].back());
    conversation_history.push_back(response_json["choices"][0]["message"]);

    return response_json;
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

bool ChatGPTClient::save_history(const string& filename) {
    ofstream ofs(filename);
    if (!ofs) {
        cerr << "Error: failed to open file " << filename << " for writing." << endl;
        return false;
    }

    try {
        ofs << setw(4) << conversation_history << endl;
        ofs.close();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        ofs.close();
        return false;
    }

    return true;
}


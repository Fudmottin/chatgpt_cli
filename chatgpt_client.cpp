#include "chatgpt_client.h"

using namespace std;
using json = nlohmann::json;

ChatGPTClient::ChatGPTClient(const string& api_key, const string& api_base_url)
    : AIClient(), // Initialize the base class
      api_key(api_key), api_base_url(api_base_url), model(supported_models[0]),
      temperature(0.7), topp(0.7), client(), chatgpt_response() {
    conversation_history = json::array();
    client.SetUrl(cpr::Url{api_base_url});
    client.SetHeader(cpr::Header{
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"},
    });
    conversation_history.push_back({{"role", "system"}, {"content", "start chat"}});
}

void ChatGPTClient::set_model(const string& model) {this->model = model;}

void ChatGPTClient::set_temperature(float temp) {temperature = temp;}

void ChatGPTClient::set_topp(float topp) {this->topp = topp;}

void ChatGPTClient::send_message(const string& message) {
        json request_data = {
	    {"model", model},
            {"messages", json::array({
                {
                    {"role", "user"},
                    {"content", message}
                }
            })},
            {"temperature", temperature}
    	};

        json response = send_request(request_data);
        chatgpt_response = extract_response(response);
}

string ChatGPTClient::get_response() {
	string s = "\nAssistent: " + chatgpt_response + "\n";
        chatgpt_response.clear();
	return s;
}

json ChatGPTClient::send_request(const json& request_data) {
    auto body = request_data.dump();
    client.SetBody(cpr::Body{body});

    conversation_history.push_back(request_data);
    auto response = client.Post();
    conversation_history.push_back(json(response.text));

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


#include "chat_client.h"

using namespace std;
using json = nlohmann::json;

ChatClient::ChatClient(const string& api_key)
    : OpenAIClient(api_key) {
    api_base_url += string("v1/chat/completions");
    model = string("gpt-3.5-turbo");
    client.SetUrl(cpr::Url{api_base_url});
}

string ChatClient::send_message(const string& message) {
        json request_data = {
	    {"model", model},
            {"messages", json::array({
                {
                    {"role", "user"},
                    {"content", message},
                }
            })},
            {"temperature", temperature},
    	};

	if (max_tokens != 0) request_data.emplace("max_tokens", max_tokens);
	if (presence_penalty != 0.0)
	    request_data.emplace("presence_penalty", presence_penalty);
	if (frequency_penalty != 0.0)
	    request_data.emplace("frequency_penalty", frequency_penalty);

	json response;
	try {
            response = send_request(request_data);
        }
	catch (...) {
            cout << "Unknown exception thrown from ChatClient::send_request()" << endl;
	    exit(0);
	}

        auto chat_response = extract_response(response);

        conversation_history.push_back(string("user: " + message + "\n"));
        conversation_history.push_back(string("AI: " + chat_response + "\n"));

	return string("\nAssitent: " + chat_response + "\n");
}

json ChatClient::send_request(const json& request_data) {
    const size_t max_context_length = 7;
    const size_t max_words = 42;
    vector<string> context = {};

    cout << "dealing with context" << endl;

    if (conversation_history.size() > max_context_length) {
	auto first = conversation_history.end() - max_context_length;
        auto last = conversation_history.end();

        context.insert(context.end(), first, last);
    } else {
        context = conversation_history;
    }

    for (size_t i = 0; i < context.size(); ++i) {
        context[i] = util::trim_content(context[i], max_words);
    }

    cout << "context dealt with. Adding new message." << endl;

    string content = request_data["messages"][0]["content"].get<string>();
    context.push_back(content);

    cout << "new message added" << endl;

    string concatenated_context;
    for (const auto& str : context) {
        concatenated_context += str + "\n";
    }

    json updated_request_data = request_data;
    updated_request_data["messages"] = json::array({
       {
           {"role", "user"},
           {"content", concatenated_context}
       }
    });

    auto body = updated_request_data.dump();

    cout << updated_request_data.dump() << endl;
	
    client.SetBody(cpr::Body{body});

    auto response = client.Post();
    
    cout << "\n" << response.text << endl;

    if (response.status_code != 200) {
        cerr << "Error: " << response.status_code << " " << response.error.message << endl;
        throw runtime_error("Request failed");
    }

    if (response.text.empty()) {
        cerr << "Error: Empty response received" << endl;
        throw runtime_error("Empty response");
    }

    json response_json = json::parse(response.text);


    return response_json;
}

string ChatClient::extract_response(const json& response) {
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

bool ChatClient::save_history(const string& filename) {
    ofstream ofs(filename);
    if (!ofs) {
        cerr << "Error: failed to open file " << filename << " for writing." << endl;
        return false;
    }

    try {
	for (auto line : conversation_history)
           ofs << line;
        ofs.close();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        ofs.close();
        return false;
    }

    return true;
}


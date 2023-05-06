#include "openai_client.h"
#include "util.h"

using namespace std;
using json = nlohmann::json;

OpenAIClient::OpenAIClient(const string& api_key)
	: client(), api_key(api_key), api_base_url(string("https://api.openai.com/")),
          temperature(1.0), max_tokens(0), presence_penalty(0.0),
          frequency_penalty(0.0), conversation_history() {
    client.SetHeader(cpr::Header{
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"},
    });
}

void OpenAIClient::set_max_tokens(int tok) {max_tokens = tok;}

void OpenAIClient::set_temperature(float temp) {
	temperature = util::clamp(temp, 0.0, 2.0);
}

void OpenAIClient::set_presence_penalty(float penalty) {
	presence_penalty = util::clamp(penalty, -2.0, 2.0);
}

void OpenAIClient::set_frequency_penalty(float penalty) {
	frequency_penalty = util::clamp(penalty, -2.0, 2.0);
}

string OpenAIClient::get_api_key() const {
    return api_key;
}


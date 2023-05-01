#include "ai_client.h"

AIClient::AIClient(const string& api_key, const string& api_base_url)
	: client(), api_key(api_key), api_base_url(api_base_url) {
    client.SetUrl(cpr::Url{api_base_url});
    client.SetHeader(cpr::Header{
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"},
    });
}


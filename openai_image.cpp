#include "openai_image.h"
#include "utils.h"
#include <cppcodec/base64_rfc4648.hpp> // Add this include

OpenAIImage::OpenAIImage(const string& api_key)
    : OpenAIClient(api_key) {
    api_base_url = "https://api.openai.com/v1/images/generations";
    image_size = "1024x1024";
}

void OpenAIImage::set_image_size(const string& size) {
    image_size = size;
}

string OpenAIImage::send_message(const string& message) {
    if (message.length() > 1000) {
        cerr << "Error: Message length exceeds 1000 characters." << endl;
        return "";
    }

    json payload = {
        {"prompt", message},
        {"n", 2},
        {"size", image_size},
        {"response_format", "b64_json"}
    };

    client.SetUrl(api_base_url);
    client.SetHeader({
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + api_key}
    });
    client.SetBody(payload.dump());

    auto response = client.Post();

    if (response.status_code == 200) {
        auto result = json::parse(response.text);
        conversation_history.push_back(message);
        string filenames = process_result(result);
        conversation_history.push_back(filenames);
        return filenames;
    } else {
        cerr << "Error: Request failed with status code " << response.status_code << endl;
        return "";
    }
}

string OpenAIImage::process_result(const json& result) {
    vector<string> filenames;
    string save_dir = util::get_chatgpt_cli_dir();
    for (const auto& entry : result["data"]) {
        string image_data_base64 = entry["image"];
        auto binary_data = cppcodec::base64_rfc4648::decode(image_data_base64);

        string file_name = save_dir + "/image_" + util::get_formatted_time() + ".png";
        ofstream image_file(file_name, ios::binary);
        image_file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
        image_file.close();

        filenames.push_back(file_name);
    }
    return join(filenames, ", ");
}

bool OpenAIImage::save_history(const string& file_name) {
    ofstream history_file(file_name);
    if (!history_file) {
        cerr << "Error: Unable to open file for writing: " << file_name << endl;
        return false;
    }

    for (const auto& entry : conversation_history) {
        history_file << entry << endl;
    }

    history_file.close();
    return true;
}

string OpenAIImage::join(const vector<string>& strings, const string& delimiter) {
    string result;
    for (size_t i = 0; i < strings.size(); ++i) {
        result += strings[i];
        if (i < strings.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}


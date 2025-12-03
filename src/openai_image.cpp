// chatgpt_cli openai_image.cpp

#include "openai_image.h"
#include "utils.h"
#include <cppcodec/base64_rfc4648.hpp>

OpenAIImage::OpenAIImage(const string& api_key)
    : OpenAIClient(api_key) {
    api_base_url = "https://api.openai.com/v1/images/generations";
    image_size = "1024x1024";
}

void OpenAIImage::set_image_size(const string& size) {
    image_size = size;
}

string OpenAIImage::send_message(const string& message) {
    string prompt = message.substr(0, 1000);
    if (message.length() > 1000) {
        cerr << "Error: Message length exceeds 1000 characters." << endl;
        cerr << "Using prompt, \"" << prompt << "\" instead." << endl;
    }

    json payload = {
        {"prompt", prompt},
        {"n", 1},
        {"size", image_size},
        {"response_format", "b64_json"}
    };

    client.SetUrl(api_base_url);
    client.SetBody(payload.dump());

    auto response = client.Post();

    if (response.status_code == 200) {
        auto result = json::parse(response.text);
        conversation_history.push_back(prompt);
        string filenames = process_result(result);
        conversation_history.push_back(filenames);
        return filenames;
    } else {
        cerr << "Error: Request failed with status code " << response.status_code << endl;

        try {
            json error_json = json::parse(response.text);
            string error_message = error_json["error"]["message"];

            cerr << "Error details: " << error_message << endl << endl;
            return "";
        } catch (const exception& e) {
            cerr << "Error parsing the error message: " << e.what() << endl;
            return response.text;
        }
    }
}

string OpenAIImage::process_result(const json& result) {
    vector<string> filenames;
    string save_dir = util::get_chatgpt_cli_dir();
    size_t counter = 0; // Add a loop counter here
    if (result.contains("data")) {
        for (const auto& entry : result["data"]) {
            if (entry.contains("b64_json")) {
                string image_data_base64 = entry["b64_json"];
                auto binary_data = cppcodec::base64_rfc4648::decode(image_data_base64);

                string file_name = save_dir + "/image_" + util::get_formatted_time() + "_" + to_string(counter) + ".png";
                ofstream image_file(file_name, ios::binary);
                image_file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
                image_file.close();

                filenames.push_back(file_name);
                counter++;
            }
        }
    }
    return join(filenames, "\n");
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


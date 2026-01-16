// clanker openai_image.h

#pragma once

#include <cpr/cpr.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "openai_client.h"
#include "utils.h"

using namespace std;
using json = nlohmann::json;

class OpenAIImage : public OpenAIClient {
 public:
   OpenAIImage(const string& api_key);
   virtual ~OpenAIImage() = default;

   virtual string send_message(const string& message) override;
   virtual bool save_history(const string& file_name) override;

   void set_image_size(const string& size);

 protected:
   string process_result(const json& result);
   string join(const vector<string>& strings, const string& delimiter);
   string image_size;
};


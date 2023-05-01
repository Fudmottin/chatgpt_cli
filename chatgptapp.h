#pragma once

#include "utils.h"

class ChatGPTApp {
public:
    ChatGPTApp(EditLine *el, History *hist) : el(el), hist(hist) {}
    ~ChatGPTApp() {
        save_history_to_file(hist);
        if (el) el_end(el);
        if (hist) history_end(hist);
    }

    // Add other methods and member variables here, such as
    // save_history_to_file(), load_history_from_file(), etc.

private:
    EditLine *el;
    History *hist;
};


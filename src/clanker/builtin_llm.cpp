#include "clanker/builtins.h"

#include <iostream>

namespace clanker {

static int bi_models(const BuiltinContext&, const Argv&) {
   std::cout << "openai:gpt-stub\nanthropic:claude-stub\n";
   return 0;
}

static int bi_use(const BuiltinContext&, const Argv& argv) {
   if (argv.size() < 2) {
      std::cerr << "use: expected <backend>\n";
      return 2;
   }
   std::cout << "default backend set to: " << argv[1] << " (stub)\n";
   return 0;
}

static int bi_prompt(const BuiltinContext&, const Argv& argv) {
   if (argv.size() < 2) {
      std::cerr << "prompt: expected text\n";
      return 2;
   }
   std::cout << "[stub llm] ";
   for (std::size_t i = 1; i < argv.size(); ++i) {
      if (i != 1) std::cout << ' ';
      std::cout << argv[i];
   }
   std::cout << "\n";
   return 0;
}

static int bi_ask(const BuiltinContext&, const Argv& argv) {
   if (argv.size() < 3) {
      std::cerr << "ask: expected <backend> <text...>\n";
      return 2;
   }
   std::cout << "[stub " << argv[1] << "] ";
   for (std::size_t i = 2; i < argv.size(); ++i) {
      if (i != 2) std::cout << ' ';
      std::cout << argv[i];
   }
   std::cout << "\n";
   return 0;
}

void add_llm_builtins(Builtins& b) {
   b.add("models", bi_models, "models — list configured model backends");
   b.add("use", bi_use, "use <backend> — select default backend (stub)");
   b.add("prompt", bi_prompt, "prompt <text...> — send text to default model (stub)");
   b.add("ask", bi_ask, "ask <backend> <text...> — send text to backend (stub)");
}

} // namespace clanker


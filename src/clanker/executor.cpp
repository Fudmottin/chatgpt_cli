// src/clanker/executor.cpp
#include <iostream>

#include "clanker/executor.h"
#include "clanker/process.h"

namespace clanker {

Executor::Executor(Builtins builtins, std::filesystem::path root)
   : builtins_(std::move(builtins))
   , root_(std::move(root)) {}

int Executor::run_simple(const SimpleCommand& cmd) {
   if (cmd.argv.empty()) return 0;

   if (auto fn = builtins_.find(cmd.argv.front())) {
      BuiltinContext ctx{.root = root_};
      return (*fn)(ctx, cmd.argv);
   }

   // Single external command (no pipeline)
   return run_external_pipeline({cmd.argv});
}

int Executor::run_pipeline(const Pipeline& pipeline) {
   if (pipeline.stages.empty()) return 0;

   if (pipeline.stages.size() == 1) return run_simple(pipeline.stages[0]);

   // Pipeline: external commands only for now.
   for (const auto& st : pipeline.stages) {
      if (st.argv.empty()) return 2;
      if (builtins_.find(st.argv.front())) {
         std::cerr << "error: built-ins in pipelines not implemented yet\n";
         return 2;
      }
   }

   std::vector<std::vector<std::string>> stages;
   stages.reserve(pipeline.stages.size());
   for (const auto& st : pipeline.stages) stages.push_back(st.argv);

   return run_external_pipeline(stages);
}

} // namespace clanker


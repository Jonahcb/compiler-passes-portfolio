#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "../utils.hpp"   // for Instruction, from_json, etc.

namespace trace {

using json = nlohmann::json;

void injectTrace(const std::string& path, const std::vector<Instruction>& guardedTrace, const std::string& outputPath){
  // Read the program from its json source file or stdin
  json j;
  if (path == "-" || path.empty()){
    std::cin >> j;
  }
  else {
    std::ifstream programFile(path);
  
    programFile >> j;
  }


  // pick out the “main” function’s instrs array
  auto& funcs = j.at("functions");
  json instrsJson;
  for (auto& f : funcs) {
    if (f.at("name") == "main") {
      instrsJson = f.at("instrs");
      break;
    }
  }
  // convert the JSON array into a vector<Instruction>
  auto allInstrs = instrsJson.get<std::vector<Instruction>>();

  // 3) Build the new instruction list
  std::vector<Instruction> newProgram;
  newProgram.reserve(1 + guardedTrace.size() + 1 + 1 + allInstrs.size());

  // a) speculate
  Instruction speculateInstr; speculateInstr.op = "speculate";
  newProgram.push_back(speculateInstr);

  // b) hot-path
  newProgram.insert(newProgram.end(),
                    guardedTrace.begin(), guardedTrace.end());

  // c) commit
  Instruction commitInstr; commitInstr.op = "commit";
  newProgram.push_back(commitInstr);

  // d) fallback label
  Instruction fallback; fallback.op = "";
  fallback.label = "hotpathfailed";
  newProgram.push_back(fallback);

  // e) original code
  newProgram.insert(newProgram.end(),
                    allInstrs.begin(), allInstrs.end());

  // 4) Replace and emit
  //   find the `main` function again and overwrite its instr list:
  for (auto& f : j["functions"]) {
    if (f["name"] == "main") {
      f["instrs"] = json::array();
      for (auto& inst : newProgram)
        f["instrs"].push_back(inst);
      break;
    }
  }

  // Write to output file
  std::ofstream outFile(outputPath);
  outFile << j.dump(2) << "\n";


}

}
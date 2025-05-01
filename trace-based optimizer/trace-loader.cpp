#include <nlohmann/json.hpp>    // for nlohmann::json
#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>
#include "../utils.hpp"

namespace trace {
using json = nlohmann::json;


std::vector<Instruction> addGuards(const std::vector<Instruction>& rawInstrs, const std::string& label){
  std::vector<Instruction> guardedInstrs;

  guardedInstrs.reserve(rawInstrs.size());

  for (const auto& instr: rawInstrs){
    // Initialize a new instr
    Instruction newInstr;
    if (instr.op == "br"){
        newInstr.op = "guard";
        newInstr.args = instr.args;
        newInstr.labels = {"hotpathfailed"};

        guardedInstrs.push_back(std::move(newInstr));
    }
    else {
      guardedInstrs.push_back(instr);
    }
  }

  return guardedInstrs;
}

std::vector<Instruction>
traceLoader(const std::string& path){

  // Read trace from file and parse the JSON into a list of instructions
  std::ifstream traceFile(path);
  json j;

  traceFile >> j; // uses utils to/from json functions for Instruction type

  // Put all the instructions into a list
  auto rawInstrs = j.get<std::vector<Instruction>>();

  // Loop through instructions and replace branches with guards
  auto guardedInstrs = addGuards(rawInstrs, "hotpathfailed");

  // Return the guarded list
  return guardedInstrs;

}

}
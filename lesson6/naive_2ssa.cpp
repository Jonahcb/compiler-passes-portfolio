#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <fstream>
#include "../lesson2/form_cfg.h"
#include "../lesson2/form_blocks.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void insert_get_phi(std::vector<json>& block, const std::string& var){ 
  // Construct the phi node 
  json phi_node = {
      {"op", "get"},
      {"dest", var},  // Unique phi name per block
  };

// Insert after the label instruction if the first instruction has "label"
if (!block.empty() && block[0].contains("label")) {
  if (block.size() > 1) {
      block.insert(block.begin() + 1, phi_node); // Insert at second position
  } else {
      block.push_back(phi_node); // If block only has a label, insert at the end
  }
} else {
  block.insert(block.begin(), phi_node); // Otherwise, insert at the start
}


}

void insert_set_phi(std::vector<json>& block, const std::string& var){
  // Construct the phi node 
  json phi_node = {
    {"op", "set"},
    {"dest", var},  // Unique phi name per block
    {"args", json::array({var})}
  };

  // Insert at the end of the block
  block.push_back(phi_node);

}

auto to_ssa (json& func){
  // form cfg
  std::vector<std::vector<json>> blocks = form_blocks(func);

  // Initialize dictionary with all the variable names
  std::unordered_set<std::string> variables;


  // loop through each basic block to grab all unique vars
  for (auto& block: blocks){
    // loop through each instruction in each basic block
    for (auto& instr: block){
      // Only grab assignments (nothing else to grab)
      if (instr.contains("dest")){
        // Grab the destination variable of the instruction
        std::string var = instr["dest"];
        
        // Add to set of variables
        variables.insert(var);

      }

    }
  }

  // loop through each basic block to add set instructions
  for (auto& block : blocks){
      // loop through each variable
      for (auto& var: variables){
  
        // at the start of every block, insert a set phi node for each variable 
        insert_set_phi(block, var);
      }
  
    }
  
  // loop through each basic block to add get instructions
  for (auto& block : blocks){
    // if the first block has no predecessor, don't insert 'gets'
    if (!block[0].contains("label") || block != blocks.begin())
    {
      // loop through each variable
      for (auto& var: variables){

      // at the start of every block, insert a get phi node for each variable 
      insert_get_phi(block, var);
    }
    }

  }

  return blocks;

}

int main() {
  // Read JSON input from stdin
  json program;
  std::cin >> program;
  std::cout << "Just read in the program\n";

  // Ensure JSON input contains functions
  if (!program.contains("functions") || program["functions"].empty()) {
      std::cerr << "Error: No functions found in the JSON input.\n";
      return 1;
  }

  // Process each function in the program
  for (auto& function : program["functions"]) {
      if (!function.contains("instrs")) {
          std::cerr << "Warning: Function '" << function["name"] << "' has no instructions.\n";
          continue;
      }

      // Convert function to SSA
      auto ssa_blocks = to_ssa(function["instrs"]);

      // Replace the function's instrs with SSA-transformed instructions
      function["instrs"] = json::array();
      for (const auto& block : ssa_blocks) {
          for (const auto& instr : block) {
              function["instrs"].push_back(instr);
          }
      }
  }

  // Define output filename
  std::string output_filename = "transformed.json";

  // Write the modified program back to a file
  std::ofstream output_file(output_filename);
  if (!output_file) {
      std::cerr << "Error: Could not open output file for writing.\n";
      return 1;
  }

  output_file << program.dump(4); // Pretty print with 4 spaces
  output_file.close();

  std::cout << "Transformed program written to '" << output_filename << "'\n";
  return 0;
}


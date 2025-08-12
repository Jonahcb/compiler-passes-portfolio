#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <nlohmann/json.hpp>
#include "form_blocks.h"

using json = nlohmann::json;

std::string generate_new_name(const std::string &prefix)
{
  // Initialize i as a static variable so it persists across all function calls
  static int i = 1;

  std::string name;

  name = prefix + std::to_string(i);

  // Incremement i for next call
  i = i + 1;

  return name;
}

std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>>
form_block_map(std::vector<std::vector<json>> &blocks)
{

  // Initialize the ordered dict via an unordered map and vector with keys to track insertion order
  std::unordered_map<std::string, std::vector<json>> block_map;
  std::vector<std::string> block_insertion_order;

  // Iterate through the blocks
  for (auto &block : blocks)
  {

    // Initialize the name for this block
    std::string name;

    // If this block starts with a label, use the label as the block name and remove the label
    if (!block.empty() && block[0].contains("label"))
    {
      name = block[0]["label"];

      // Remove the first instruction since it is just a label
      block.erase(block.begin());
    }
    // If this block does not start with a label, give it a unique name
    else
    {
      name = generate_new_name("b"); // idk what I should feeed into this
    }

    // Store the block in the block map with its new name
    block_map[name] = block;

    // Insert its name (key) into the insertion order tracking vector
    block_insertion_order.push_back(name);
  }

  return {block_map, block_insertion_order};
}

std::vector<std::string> get_successors(const json &instr)
{

  // Check that the instruction is has an opcode
  if (!instr.contains("op") || !instr["op"].is_string())
  {
    throw std::runtime_error("Instruction is missing a valid 'op' field.");
  }

  // If the instruction is a branch or jump, add its successors (this assumes every instruction has "op")
  if (instr["op"] == "br" || instr["op"] == "jmp")
  {
    return instr["labels"].get<std::vector<std::string>>();
  }
  else if (instr["op"] == "ret")
  {
    return {};
  }
  // Raise an error if the instruction is not a terminator (branch, jump, or return)
  else
  {
    throw std::runtime_error("Instruction is not a terminator. Cannot retrieve successors.");
  }
}

std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>>
add_terminators(std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>> &ordered_block_map)
{

  // Initialize counter to know where I am in the loop
  int i = 0;

  // Loop through all basic blocks (access the first element of the pair passed in)
  for (auto &[block_name, block] : ordered_block_map.first)
  {

    // Case 1: if the basic block is empty
    if (block.empty())
    {
      // Case 1.1: if the current basic block is the last basic block in the function
      if (i == ordered_block_map.second.size() - 1)
      {
        // Add a return call
        block.push_back(json{{"op", "ret"}, {"args", json::array()}});
      }
      // Case 1.2: if the current basic block is not the last basic block in the function
      else
      {
        // Add a jump instruction to the next basic block
        std::string dest = ordered_block_map.second[i + 1];
        block.push_back(json{{"op", "jmp"}, {"labels", json::array({dest})}});
      }
    }

    // Case 2: if the basic block is not empty and if the last instruction is not a terminator
    else if (block.back()["op"] != "jmp" && block.back()["op"] != "br" && block.back()["op"] != "ret")
    {
      // Case 2.1: if the current basic block is the last basic block in the function
      if (i == ordered_block_map.second.size() - 1)
      {
        // Add return call
        block.push_back(json{{"op", "ret"}, {"args", json::array()}});
      }
      // Case 2.2: if the current basic block is not the last basic block in the function
      else
      {
        // Add a jump instruction to the next basic block
        std::string dest = ordered_block_map.second[i + 1];
        block.push_back(json{{"op", "jmp"}, {"labels", json::array({dest})}});
      }
    }
    // Update the counter
    i = i + 1;
  }
  return ordered_block_map;
}

std::pair<std::unordered_map<std::string, std::vector<std::string>>,
          std::unordered_map<std::string, std::vector<std::string>>>
edges(std::pair<std::unordered_map<std::string, std::vector<json>>,
                std::vector<std::string>> &ordered_block_map)
{
  // Initialize predecessors and successors maps
  std::unordered_map<std::string, std::vector<std::string>> predecessors;
  std::unordered_map<std::string, std::vector<std::string>> successors;

  // Through through all the blocks
  for (auto &[block_name, block] : ordered_block_map.first)
  {
    // Through through all the successors of the current block by checking successors of the last instr
    for (auto &succ : get_successors(block.back()))
    {
      // This successor is a successor of the current block
      successors[block_name].push_back(succ);

      // This current block is a predecessor of the successor
      predecessors[succ].push_back(block_name);
    }
  }

  return {predecessors, successors};
}

/*
int main() {
    // Read JSON input from stdin
    json program;
    std::cin >> program;
    std::cout << "Just read in the program\n";

    // Check if the JSON has a "functions" field
    if (!program.contains("functions") || !program["functions"].is_array()) {
        std::cerr << "Error: JSON input must contain a 'functions' array.\n";
        return 1;
    }

    // Assume we are processing the first function in the array
    json function = program["functions"][0]["instrs"];

    std::cout << "About to form blocks\n";
    std::vector<std::vector<json>> blocks = form_blocks(function);
    std::cout << "Just formed blocks\n";

    // Print out the block map
    std::cout << "{\n  \"blocks\": {\n";
    for (const auto& block : blocks) {
        for (const auto& instr : block) {
            std::cout << "      " << instr.dump() << ",\n";
        }
        std::cout << "    ],\n";
    }
    std::cout << "  },\n";


    // Step 1: Create the block map
    auto ordered_block_map = form_block_map(blocks);
    std::cout << "Just formed the block map\n";

    // Print out the block map
    std::cout << "{\n  \"blocks\": {\n";
    for (const auto& [name, block] : ordered_block_map.first) {
        std::cout << "    \"" << name << "\": [\n";
        for (const auto& instr : block) {
            std::cout << "      " << instr.dump() << ",\n";
        }
        std::cout << "    ],\n";
    }
    std::cout << "  },\n";

    // Step 2: Ensure all blocks have terminators
    ordered_block_map = add_terminators(ordered_block_map);
    std::cout << "Just added the terminators\n";

    // Print out the block map
    std::cout << "{\n  \"blocks\": {\n";
    for (const auto& [name, block] : ordered_block_map.first) {
        std::cout << "    \"" << name << "\": [\n";
        for (const auto& instr : block) {
            std::cout << "      " << instr.dump() << ",\n";
        }
        std::cout << "    ],\n";
    }
    std::cout << "  },\n";

    // Step 3: Generate the predecessors and successors maps
    auto [predecessors, successors] = edges(ordered_block_map);
    std::cout << "Just computed the edges\n";

    // Print out the block map
    std::cout << "{\n  \"blocks\": {\n";
    for (const auto& [name, block] : ordered_block_map.first) {
        std::cout << "    \"" << name << "\": [\n";
        for (const auto& instr : block) {
            std::cout << "      " << instr.dump() << ",\n";
        }
        std::cout << "    ],\n";
    }
    std::cout << "  },\n";

    // Print out the predecessors
    std::cout << "  \"predecessors\": {\n";
    for (const auto& [name, preds] : predecessors) {
        std::cout << "    \"" << name << "\": " << json(preds).dump(2) << ",\n";
    }
    std::cout << "  },\n";

    // Print out the successors
    std::cout << "  \"successors\": {\n";
    for (const auto& [name, succs] : successors) {
        std::cout << "    \"" << name << "\": " << json(succs).dump(2) << ",\n";
    }
    std::cout << "  }\n}\n";

    return 0;
}
*/

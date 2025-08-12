#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <nlohmann/json.hpp> // Include the nlohmann/json library I don't understand this include
#include "form_blocks.h"

using json = nlohmann::json;

std::vector<std::vector<json>> form_blocks(std::vector<json> func)
{
  // Initialize the list of blocks
  std::vector<std::vector<json>> blocks;

  // Start with an empty block and add instructions to it
  std::vector<json> block;

  // Loop through all the instructions in this function
  for (auto instr : func)
  {

    // If it is an instruction, just add it to the block
    if (instr.contains("op"))
    {
      block.push_back(instr);

      // If this instruction changes the control flow, end current block
      if (instr["op"] == "jmp" || instr["op"] == "br" || instr["op"] == "ret")
      {

        blocks.push_back(block);

        // Start a new block
        block.clear();
      }
    }
    // If it is not an instruction, it must be a label, so it must begin its own block
    else
    {
      if (!block.empty()) {
        blocks.push_back(block);
    }

      // Start a new block
      block.clear();

      // Append the label to the top of the block
      block.push_back(instr);
    }
  }

  // Add final block if needed
  if (!(block.empty()))
  {
    blocks.push_back(block);
  }

  return blocks;
}

void print_block(json& prog)
{
  // Iterate through functions in the prog
  for (auto& [func_name, func] : prog["functions"].items()) {
    std::cout << "Processing function: " << func_name << "\n";
    // Form basic blocks out of the function
    std::vector<std::vector<json>> blocks = form_blocks(func["instrs"]);

    int block_id = 0;
    for (const auto& block : blocks) {
        std::cout << "Basic Block " << block_id++ << ":\n";
        
        // Print each instruction inside the block
        for (const auto& instr : block) {
            std::cout << instr.dump() << "\n";  // Pretty-print JSON
        }
        
        std::cout << "--------------------\n";
    }
  }

  

}

/* int main()
{
  // Initialize json object from package
  json program;

  // Read in input
  std::cin >> program;

  // Feed it into print_block
  print_block(program);

  // Return exit code
  return 0;
} */


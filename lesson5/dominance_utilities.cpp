#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include "../lesson2/form_cfg.h"
#include "../lesson2/form_blocks.h"


std::vector<std::string> find_common_dominators(const std::vector<std::string> &predecessors, 
  const std::unordered_map<std::string, std::vector<std::string>>& dominators_list_list)
{

  // Initialize counts map
  std::unordered_map<std::string, int> counts;

  // Initialize common map
  std::vector<std::string> common;

  // Loop through the list of predecessors
  for (size_t i = 0; i < predecessors.size(); i++)
  {

    // Loop through each predecessor and increment dominator count in dominator count matrix
    for (auto dominator : dominators_list_list.at(predecessors[i]))
    {
      // If the dominator is also in count, add it
      if (counts.count(dominator))
      {
        counts[dominator] += 1;
      }
      else
      {
        counts[dominator] = 1;
      }
    }
  }

    // if the count equals the number of preds, then this dominator dominates every predecessor
    for (auto [dominator_name, dominator_count] : counts)
    {
      if (dominator_count == predecessors.size())
      {
        common.push_back(dominator_name);
      }
    }

  return common;
}


/**
 * @brief Finds the dominators in a function.
 *
 * This function takes a BRIL function and returns the dominators.
 *
 * @param func BRIL function to analyze.
 * @return a map that maps block names to names of blocks that dominate that block.
 */
std::unordered_map<std::string, std::vector<std::string>> find_dominators(std::vector<json> func)
{
  
  
  // Initialize map of block to sets of blocks
  std::unordered_map<std::string, std::vector<std::string>> dominators_list_list;

  // Break the first function down into basic blocks
  std::vector<std::vector<json>> blocks = form_blocks(func);
  std::cout << "This is working ";

  // Turn into full cfg
  std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>>
      block_map =
          form_block_map(blocks);

  // Add terminators
  block_map = add_terminators(block_map);

  // Get successors and predecessors
  std::pair<std::unordered_map<std::string, std::vector<std::string>>,
            std::unordered_map<std::string, std::vector<std::string>>>
      preds_succs =
          edges(block_map);

  // Initialize every block with just itself as its dominator (this loop might be unnecessary)
  for (auto& [block_name, block] : block_map.first)
  {
    dominators_list_list[block_name] = {block_name};
  }

  // Initialize changing flag
  bool changing = true;

  // While not all dominators have been found
  while (changing)
  {
    // Reset flag
    changing = false;

    // Loop through every block and recompute its dominators
    for (auto& [block_name, dominators_list] : dominators_list_list)
    {

      // compute dominators inherited from predecessors
      std::vector<std::string> new_dominators = find_common_dominators(preds_succs.first[block_name], dominators_list_list); // fix this

      // Add this vertex and the intersection of all the dominators of this vertex's predecessors
      new_dominators.push_back(block_name);

      // check if the list has changed since last time, if not, we are done
      if (new_dominators != dominators_list)
      {
        changing = true;
        dominators_list_list[block_name] = new_dominators;
      }
    }
  }

  return dominators_list_list;
}


int main()
{
    // Read JSON input
    json program;
    std::cin >> program;

    // Check if "functions" exists and is an array
    if (!program.contains("functions") || !program["functions"].is_array())
    {
        std::cerr << "Error: Expected a 'functions' key with an array of functions.\n";
        return 1;
    }

    for (auto& [func_name, func] : program["functions"].items()) {
      std::cout << "Processing function: " << func_name << "\n";
      // Form basic blocks out of the function
      std::vector<std::vector<json>> blocks = form_blocks(func["instrs"]);
      std::cout << "Failing before here";

        std::string function_name = func["name"];

        // Compute dominators for the current function
        std::unordered_map<std::string, std::vector<std::string>> dominators = find_dominators(func["instrs"]);

        // Print function name
        std::cout << "Function: " << func_name << "\n";

        // Print each block with its list of dominators
        for (const auto& [block, dominator_list] : dominators)
        {
            std::cout << "  Block: " << block << "\n  Dominators: ";
            for (const auto& dominator : dominator_list)
            {
                std::cout << dominator << " ";
            }
            std::cout << "\n\n";
        }
    }

    return 0;
}

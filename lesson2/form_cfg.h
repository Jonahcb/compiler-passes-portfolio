#ifndef FORM_CFG_H
#define FORM_CFG_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "form_cfg.h"

using json = nlohmann::json;

// Function to generate a unique block name
std::string generate_new_name(const std::string& prefix);

// Function to map blocks with insertion order
std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>> 
form_block_map(std::vector<std::vector<json>>& blocks);

// Function to get successors of an instruction
std::vector<std::string> get_successors(const json& instr);

// Function to add terminators to basic blocks
std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>> 
add_terminators(std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>>& ordered_block_map);

// Function to compute edges in the control flow graph
std::pair<std::unordered_map<std::string, std::vector<std::string>>, 
          std::unordered_map<std::string, std::vector<std::string>>> 
edges(std::pair<std::unordered_map<std::string, std::vector<json>>, std::vector<std::string>>& ordered_block_map);

#endif // FORM_CFG_H
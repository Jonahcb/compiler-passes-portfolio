#ifndef FORM_BLOCKS_H
#define FORM_BLOCKS_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Function to form basic blocks from a function
std::vector<std::vector<json>> form_blocks(std::vector<json> func);

// Function to print the blocks in a formatted way
void print_block(json& prog);

#endif // FORM_BLOCKS_H
#include "utils.hpp"
#include <nlohmann/json.hpp>    // for nlohmann::json

using nlohmann::json;


void to_json(nlohmann::json& j, const Instruction& i) {
  if (i.label) {
    j = nlohmann::json{{"label", *i.label}};
  } else {
    j = nlohmann::json{{"op", i.op}};
    if (i.dest)  j["dest"]   = *i.dest;
    if (i.type)  j["type"]   = *i.type;
    if (!i.funcs.empty()) j["funcs"]  = i.funcs;
    if (!i.args.empty())  j["args"]   = i.args;
    if (!i.labels.empty())j["labels"] = i.labels;
    if (i.value)          j["value"]  = *i.value;
  }
}

void from_json(const nlohmann::json& j, Instruction& i) {
  if (j.contains("label")) {
    i.label = j.at("label").get<std::string>();
    //i.op.reset();
  } else {
    i.op = j.at("op").get<std::string>();
    i.label.reset();
  }
  if (j.contains("dest")) i.dest = j.at("dest").get<std::string>();
  else                   i.dest.reset();
  if (j.contains("type")) i.type = j.at("type").get<std::string>();
  else                   i.type.reset();
  if (j.contains("funcs")) i.funcs = j.at("funcs").get<std::vector<std::string>>();
  else                     i.funcs.clear();
  if (j.contains("args"))  i.args   = j.at("args").get<std::vector<std::string>>();
  else                     i.args.clear();
  if (j.contains("labels"))i.labels = j.at("labels").get<std::vector<std::string>>();
  else                     i.labels.clear();
  if (j.contains("value")) i.value  = j.at("value");
  else                     i.value.reset();
}


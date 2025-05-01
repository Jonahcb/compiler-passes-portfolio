#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>


/// \struct Instruction
/// \brief Represents either a Bril instruction or a label in a function body.
///
/// An Instruction is either:
/// - A label (if \c label has a value), in which case \c op is empty.
/// - An operation (if \c op is nonempty), in which case \c label is empty.
///
/// For operations, you may have:
/// - \c dest: the destination register name (e.g. `"x"`).
/// - \c type: the type of \c dest (e.g. `"int"`, `"bool"`).
/// - \c args: a list of operand register names or literal strings.
/// - \c funcs: a list of function names for `call` instructions.
/// - \c labels: a list of branch/guard targets.  For \c br this is two labels
///   (true, false); for \c guard it is a single fall-back label.
/// - \c value: for constant instructions (\c op == `"const"`), the literal
///   value stored as JSON (so we can represent ints, floats, strings, etc.).
struct Instruction {
  /// The operation code (e.g. "add", "mul", "br", "guard", "ret").
  /// Empty string if this is a label instruction.
  std::string                  op;

  /// If this is a label, its name (e.g. "loop", "exit").
  /// Otherwise \c std::nullopt.
  std::optional<std::string>   label;

  /// The destination register for an operation, if any.
  /// E.g. in `add x y z`, dest=="x".
  std::optional<std::string>   dest;

  /// The type of the destination register, if any.
  /// E.g. "int", "bool", or "float".
  std::optional<std::string>   type;

  /// The operand registers or literal arguments for this instruction.
  /// E.g. in `add x y z`, args=={"y","z"}.
  std::vector<std::string>     args;

  /// The target labels for branch or guard instructions.
  /// - For a two-way branch (`br`), this contains exactly two labels:
  ///   the "true" target, then the "false" target.
  /// - For a one-way guard (`guard`), this contains exactly one label:
  ///   where to de-optimize on failure.
  std::vector<std::string>     labels;

  /// The list of function names for call instructions.
  /// For a `call` operation, this vector contains exactly one element:
  ///// the name of the function being invoked.
  std::vector<std::string>     funcs;

  /// The literal value for `const` instructions.
  /// Stored as a JSON value so we can handle numbers, strings, booleans, etc.
  std::optional<nlohmann::json> value;
};

/// \brief Serialize an Instruction into a JSON object.
///
/// This function is found via argument-dependent lookup by nlohmann::json,
/// so you can write:
///   nlohmann::json j = instr;
///
/// \param j  The JSON output to populate.
/// \param i  The Instruction to serialize.
void to_json(nlohmann::json& j, const Instruction& i);

/// \brief Deserialize a JSON object into an Instruction.
///
/// This function is found via argument-dependent lookup by nlohmann::json,
/// so you can write:
///   Instruction instr = j.get<Instruction>();
///
/// \param j  The JSON input to read from.
/// \param i  The Instruction to populate.
void from_json(const nlohmann::json& j, Instruction& i);

#endif // UTILS_HPP
#ifndef TRACE_LOADER_HPP
#define TRACE_LOADER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include "../utils.hpp"    /**< Defines Instruction, to_json/from_json, etc. */

namespace trace {

/**
 * @brief Wraps branch instructions in guards.
 *
 * Iterates over an existing list of Bril instructions and replaces every
 * `br` (branch) instruction with a `guard` instruction.  The new guard
 * will test the same arguments and jump to the provided failure label
 * on guard violation.
 *
 * @param rawInstrs
 *   The original instruction sequence to process.
 * @param label
 *   The label to jump to when a guard fails (e.g., the fallback path).
 * @return
 *   A new vector of `Instruction` in which all branch instructions have
 *   been replaced by guards targeting `label`.
 */
std::vector<Instruction>
addGuards(
    const std::vector<Instruction>& rawInstrs,
    const std::string& label
);


/**
 * @brief Loads a JSON “trace” file and applies branch guards.
 *
 * Opens the trace file at the given path, parses it as a JSON array of
 * `Instruction` (using the `from_json` helper), then calls `addGuards`
 * to replace every `br` with a `guard` targeting `"hotpathfailed"`.
 *
 * @param path
 *   Filesystem path to the JSON trace.  The file is expected to contain
 *   a JSON array of instructions, e.g. `[ { "op": "add", ... }, ... ]`.
 * @return
 *   A vector of `Instruction` representing the guarded trace.
 *
 * @throws std::runtime_error
 *   If the file cannot be opened, or if JSON parsing fails.
 */
std::vector<Instruction>
traceLoader(
    const std::string& path
);

} // namespace trace

#endif // TRACE_LOADER_HPP

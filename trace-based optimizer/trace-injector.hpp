#ifndef TRACE_INJECTOR_HPP
#define TRACE_INJECTOR_HPP

#include <string>
#include <vector>
#include "../utils.hpp"   /**< Defines Instruction, from_json, to_json, etc. */

namespace trace {

/**
 * @brief Inserts a speculative “hot-path” trace into a Bril program.
 *
 * This function reads a Bril program in JSON format from the given
 * input path, locates the `"main"` function, and replaces its instruction
 * sequence with a new sequence that:
 *   1. Begins with a `speculate` instruction.
 *   2. Includes the provided guarded trace (the “hot path”).
 *   3. Ends with a `commit` instruction.
 *   4. Defines a fallback label `hotpathfailed` for trace failures.
 *   5. Appends the original `"main"` instructions as the fallback path.
 *
 * The modified program is then written out in pretty-printed JSON form
 * to the specified output path.
 *
 * @param path
 *   Filesystem path to the input Bril program (JSON file). The file
 *   must contain a top-level `"functions"` array with an entry whose
 *   `"name"` field equals `"main"`.
 *
 * @param guardedTrace
 *   A vector of `Instruction` objects representing the hot-path trace
 *   to inject between the `speculate` and `commit` markers.
 *
 * @param outputPath
 *   Filesystem path where the transformed Bril program JSON will be
 *   written. If the file exists, it will be overwritten.
 *
 * @throws std::runtime_error
 *   Thrown if the input file cannot be opened, if the JSON is malformed,
 *   or if no `"main"` function is found.
 */
void injectTrace(
    const std::string& path,
    const std::vector<Instruction>& guardedTrace,
    const std::string& outputPath
);

} // namespace trace

#endif // TRACE_INJECTOR_HPP

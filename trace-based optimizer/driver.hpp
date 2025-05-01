#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <string>
#include <vector>
#include "../utils.hpp"              /**< Defines Instruction, to_json/from_json, etc. */
#include "trace-injector.hpp"    /**< Declares injectTrace(...) */
#include "trace-loader.hpp"      /**< Declares traceLoader(...) */

/**
 * @brief Loads a hot-path trace and injects it into a Bril program.
 *
 * This function serves as the high-level driver for the trace injection pipeline.
 * It reads a “hot-path” trace from the given JSON file, wraps any branches
 * in guards (via traceLoader), then injects that guarded trace into the
 * specified Bril program (via injectTrace).  The modified program is
 * written to the hard-coded path `"data/output"`.
 *
 * @param programPath
 *   Filesystem path to the input Bril program (JSON).  Must contain a
 *   top-level `"functions"` array with a function named `"main"`.
 *
 * @param hotPathPath
 *   Filesystem path to the JSON trace file.  The file must be a JSON array
 *   of `Instruction` objects (e.g., `[ { "op": "add", … }, … ]`).
 *
 * @return
 *   Exit status code (currently always returns 1).
 *
 * @throws std::runtime_error
 *   Propagates any I/O or JSON parsing errors from `traceLoader` or
 *   `injectTrace`.
 */
int driver(
    const std::string& programPath,
    const std::string& hotPathPath
);

#endif // DRIVER_HPP

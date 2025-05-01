#include "trace-injector.hpp"
#include "trace-loader.hpp"
#include "driver.hpp"
#include <iostream>  

int main(int argc, char** argv) {
  std::string progPath  = "-";         // "-" means read JSON from stdin
  std::string hotPath   = "my.trace";  // default hot-path

  if (argc == 2) {
    // one argument → program file
    progPath = argv[1];
  } else if (argc == 3) {
    // two arguments → program file & hot-path
    progPath = argv[1];
    hotPath  = argv[2];
  } else if (argc > 3) {
    std::cerr << "Usage:\n"
              << "  # JSON on stdin, default hot-path:\n"
              << "    ./trace_driver\n\n"
              << "  # JSON on stdin, override hot-path:\n"
              << "    ./trace_driver <hot.trace>\n\n"
              << "  # program.json only (default hot-path):\n"
              << "    ./trace_driver program.json\n\n"
              << "  # program.json and hot-path:\n"
              << "    ./trace_driver program.json hot.trace\n";
    return 1;
  }
  return driver(progPath, hotPath);
}

int driver(const std::string& programPath, const std::string& hotPathPath)
{
  std::vector<Instruction> guardedInstrs = trace::traceLoader(hotPathPath);

  trace::injectTrace(programPath, guardedInstrs, "output");

  return 0;

}

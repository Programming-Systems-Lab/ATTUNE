#ifndef RR_UNWINDER_H_
#define RR_UNWINDER_H_

#include <unistd.h>
#include <vector>
#include <libunwind.h>
namespace rr {

  class Unwinder {
  public:

    // returns vector of the program counters that triggered up to that point
    static std::vector<unw_word_t> unwind_pc(pid_t pid);

    //prints the stacktrace as pc_value: (func_name+offset)
    static void print_stacktrace(pid_t pid);
  };
} // namespace rr
#endif

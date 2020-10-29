/* Does the stack unwinding */

#include <libunwind-ptrace.h>
#include "log.h"
#include "Unwinder.h"

using namespace std;

namespace rr {

  // maybe TODO modify this function to return both the function name
  // as well as the memory address where applicable 
  std::vector<unw_word_t> Unwinder::unwind_pc(pid_t pid) {
    std::vector<unw_word_t> res;
    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

    void *context = _UPT_create(pid);
    unw_cursor_t cursor;
    if (unw_init_remote(&cursor, as, context) != 0)
    {
      //TODO Figure out why this if statement triggers some times
      LOG(debug) << "couldn't initialiaze remote for unwinding";
    }

// DEBUG prlint stacktrace with eh info
//     do {
//         unw_word_t offset, pc;
//         char sym[4096];
//         if (unw_get_reg(&cursor, UNW_REG_IP, &pc))
//           LOG(debug) << "ERROR: cannot read program counter\n";
//         printf("0x%lx: ", pc);
//         if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
//           printf("(%s+0x%lx)\n", sym, offset);
//         else
//           printf("\n");
//
//         res.push_back(pc);
//       } while (unw_step(&cursor) > 0);
//       printf("-----------------\n");
// _UPT_destroy(context);
// end of DEBUG

    // unwind the program counter values on the stack
    do {
      unw_word_t pc;
      if(unw_get_reg(&cursor, UNW_REG_IP, &pc))
      LOG(debug) << "Error can't read program counter";
      res.push_back(pc);

    } while (unw_step(&cursor) > 0);

    return res;
  }

  void Unwinder::print_stacktrace(pid_t pid) {
    /* doing stack unwinding */
    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);


    void *context = _UPT_create(pid);
    unw_cursor_t cursor;
    if (unw_init_remote(&cursor, as, context) != 0)
    {
      LOG(debug) << "Couldn't initialize for remote unwinding -- process probably exited";
    }

    do {
      unw_word_t offset, pc;
      char sym[4096];
      if (unw_get_reg(&cursor, UNW_REG_IP, &pc))
      LOG(debug) << "ERROR: cannot read program counter\n";

      printf("0x%lx: ", pc);

      if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
      printf("(%s+0x%lx)\n", sym, offset);
      else
      printf("\n");
    } while (unw_step(&cursor) > 0);
    printf("-----------------\n");
    _UPT_destroy(context);
    /* The unwinding has happened */
  }
}

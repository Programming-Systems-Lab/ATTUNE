#ifndef RR_DwarfReader_H_
#define RR_DwarfReader_H_


//#include </usr/local/include/dwarf.h>
//#include </usr/local/include/libdwarf.h>
#include </home/ant/rr_muplay/obj/dependencies/libdwarf-obj/include/dwarf.h>
#include </home/ant/rr_muplay/obj/dependencies/libdwarf-obj/include/libdwarf.h>

#include <vector>

#include <libunwind.h>
#include <string>
namespace rr {
  class DwarfReader {
  public:
    /* Functions */

    /* Safe call to get_src_line that checks for special cases and errors thrown during dwarf_init */
    static std::string safe_get_src_line(const char* elf_file_path, unw_word_t mem_address);
    /* Resolves the src line associated with the given memory addres */
    static std::string get_src_line(const char* elf_file_path, unw_word_t mem_address);


    /*safe call to get_lineno_addr that checks for debug info and special cases */
    static std::vector<long> safe_get_lineno_addr(const char* elf_file_path,
                                           int line_no);

    /*
       returns a vector of memory addresses that correspond to the locations
       in the elf file that correspond to the given line
     */
    static std::vector<long> get_lineno_addrs(const char* elf_file_path, int line_no);

    /* Checks if result of libdwarf call resulted in not found or error */
    static bool check_libdwarf_error(int return_status);

    /* Determines if the file at elf_file_path has debugging information */
    static bool check_for_dwarf_info(const char* elf_file_path);

    /* error handler for the dwarf_init called in check_for_dwarf_info */
    static void check_dwarf_info_init_handler(Dwarf_Error error, Dwarf_Ptr errarg);

    /* Returns cu_die of the given elf file */
    static int get_cu_die(const char* elf_file_path, Dwarf_Die cu_die);
  };


} // namespace rr
#endif

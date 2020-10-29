/* Reads the dwarf information to find location of source files */

// TODO figure out how to not make this a full path
#include </home/ant/rr_muplay/obj/dependencies/libdwarf-obj/include/dwarf.h>
//#include <dwarf.h>
#include <libdwarf.h>

#include <vector>

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#include <sys/types.h> /* For open() */
#include <sys/stat.h>  /* For open() */
#include <fcntl.h>     /* For open() */
#include <stdlib.h>     /* For exit() */
#include <unistd.h>     /* For close() */
#include <string.h> /*for memset*/


#include "DwarfReader.h"
#include "log.h"



using namespace std;

namespace rr {

  std::vector<long> DwarfReader::safe_get_lineno_addr(const char* elf_file_path,
                                         int line_no)
  {
    std::vector<long> mem_addrs;
    if (!check_for_dwarf_info(elf_file_path))
      return mem_addrs;
    return get_lineno_addrs(elf_file_path, line_no);
  }

  /* SAME AS get_src_line except for an if statement */
  /* returns empty vector on error */
  std::vector<long> DwarfReader::get_lineno_addrs(const char* elf_file_path,
                                    int line_no)
  {
    int res;
    Dwarf_Error *errp = 0;
    char* cu_name;
    std::vector<long> mem_addrs;
    Dwarf_Die cu_die = 0;
    res = get_cu_die(elf_file_path, cu_die);

    if(check_libdwarf_error(res))
      return mem_addrs;

    res = dwarf_diename(cu_die, &cu_name, errp);
    if(check_libdwarf_error(res))
      FATAL()<< "ERROR getting cu_name";

    LOG(debug) << "The cu name is: " << cu_name;

    if(line_no > 0) {}
    return mem_addrs;
  }

  /* Returns cu_die of the given elf file in cu_die */
  /* Returns 0 on success and -1 on error */
  /* cu_die needs to be freed after function */
  int DwarfReader::get_cu_die(const char* elf_file_path, Dwarf_Die cu_die) {
    if (elf_file_path) {}
    if (cu_die) {}


    return 0;
  }

  std::string DwarfReader::safe_get_src_line(const char* elf_file_path,
                                             unw_word_t mem_address)
  {
    if(!check_for_dwarf_info(elf_file_path))
      return "No DWARF info available";
    return get_src_line(elf_file_path, mem_address);
  }

  std::string DwarfReader::get_src_line(const char* elf_file_path,
                                        unw_word_t mem_address)
  {
    Dwarf_Debug dbg = 0;
    Dwarf_Die cu_die = 0;
    Dwarf_Die no_die = 0;

    int res = DW_DLV_ERROR;

    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;
    Dwarf_Error *errp = 0;

    // for next_cu_header
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;

    // to get cu name
    char* cu_name;

    //for line number operations
    Dwarf_Signed line_cnt;
    Dwarf_Line *linebuf;
    Dwarf_Addr line_addr;
    Dwarf_Unsigned line_no;


    /* opening the file and initializing dwarf info */
    int fd = open(elf_file_path, O_RDONLY);

    /* TODO add error handline */
    res = dwarf_init(fd, DW_DLC_READ, errhand, errarg, &dbg, errp);
    if(check_libdwarf_error(res))
      return "Error during dwarf_init...no debug_info";

    //TODO see how this needs to be built to run more than one source file
    // probably need to include some sort of for/while loop to get each cu

    res = dwarf_next_cu_header(dbg, &cu_header_length,
                               &version_stamp,
                               &abbrev_offset,
                               &address_size,
                               &next_cu_header,
                               errp);

    if(check_libdwarf_error(res))
      return "No cu header to parse...no debug info";

    res = dwarf_siblingof(dbg, no_die, &cu_die, errp);

    if(check_libdwarf_error(res))
      FATAL() << "ERROR getting sibling of";

    res = dwarf_diename(cu_die, &cu_name, errp);
    if(check_libdwarf_error(res))
      FATAL()<< "ERROR getting cu_name";


    if ((res = dwarf_srclines(cu_die, &linebuf,&line_cnt, errp)) == DW_DLV_OK) {
      for (int i = 0; i < line_cnt; ++i) {
        /* Get the address of the line */
        res = dwarf_lineaddr(linebuf[i], &line_addr, errp);
        if(check_libdwarf_error(res))
          LOG(debug) << "ERROR getting lineaddr";

        /* get the line number */
        res = dwarf_lineno(linebuf[i], &line_no, errp);
        if(check_libdwarf_error(res))
        {
          LOG(debug) << "ERROR getting line number";
        }

        // LOG(debug) << cu_name << ":" << line_no << " corresponds to : 0x" << std::hex << line_addr;

        /* If the address matches the address we're looking for you have
           you have found the line number */
        if(line_addr == mem_address)
        {
          std::string res_string = std::to_string(line_no);
          return res_string.c_str();
        }

        /* de allocate line_buf where no longer in use */
        dwarf_dealloc(dbg, linebuf[i], DW_DLA_LINE);
      }
      dwarf_dealloc(dbg, linebuf, DW_DLA_LIST);
    }

    /* De allocating memory from Dwarf_Init */
    dwarf_finish(dbg, errp);
    /* closing the opened file */
    close(fd);
    return "src line not found";
  }

  bool DwarfReader::check_libdwarf_error(int return_status) {
    // TODO change this to something different for not found
    /* returns error for both not found and error */
    if(return_status == DW_DLV_ERROR ||
       return_status == DW_DLV_NO_ENTRY)
       return true;
    return false;
  }

  /* returns true if has debug info and false otherwise
     has some special cases that are ignored as well */
  bool DwarfReader::check_for_dwarf_info(const char* elf_file_path) {
    Dwarf_Handler errhand = &check_dwarf_info_init_handler;
    Dwarf_Ptr errarg = 0;
    Dwarf_Error* errp = 0;
    std::string elf_string(elf_file_path);

    /* SPECIAL CASE CHECKING */
    // TODO make the special case paths a class variable
    // and move the special case checking to a different function
    /* special case to ignore rr_page files that throw error on init_dbg */
    size_t found = elf_string.find("rr_page_");
    if (found != std::string::npos)
      return false;
    /* Make sure the file name isn't empty */
    if (elf_string.empty())
      return false;
    /* end of special case checking */

    /* opening the file and initializing dwarf info */
    int fd = open(elf_file_path, O_RDONLY);
    Dwarf_Debug dbg;
    /* TODO add error handline */
    LOG(debug) << "file_name: " << elf_file_path;
    int res = dwarf_init(fd, DW_DLC_READ, errhand, errarg, &dbg, errp);
    close(fd);
    if(res == DW_DLV_NO_ENTRY || res == DW_DLV_ERROR){
      return false;
    }
    return true;
  }

  /* Does nothing if error thrown and above function checks for error code */
  void DwarfReader::check_dwarf_info_init_handler(Dwarf_Error error, Dwarf_Ptr errarg)
  {
    if(error) {}
    if(errarg){}
  }
} // namespace rr

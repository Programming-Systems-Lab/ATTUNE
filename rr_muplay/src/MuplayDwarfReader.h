#ifndef RR_MUPLAY_DWARF_READER_H_
#define RR_MUPLAY_DWARF_READER_H_

#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include <libdwarf.h>
//TODO make this a local path
#include </home/ant/rr_muplay/obj/dependencies/libdwarf-obj/include/dwarf.h>
#include <sys/types.h> /* For open() */
#include <sys/stat.h>  /* For open() */
#include <fcntl.h>     /* For open() */
#include <stdlib.h>     /* For exit() */
#include <unistd.h>     /* For close() */
#include <string.h> /*for memset*/
#include <fcntl.h>    /* For O_RDWR */
#include <err.h>    /* For errx()*/
#include <algorithm> /*For find()*/
#include "MuplayBinaryModificationSummary.h"
#include "MuplayTypes.h"
namespace rr{

class MuplayDwarfReader
{
public:
	MuplayDwarfReader() = default;

    void find_dwarf_die(std::string &src_file_name, Dwarf_Debug *dbg, Dwarf_Die *die);

	bool check_for_dwarf_info(const char* elf_file_path);

	MuplayBinaryModificationSummary get_summary(std::string patch_src_file,
                                                std::string dwarf_src_file,
                                                std::string elf_file_path,
                                                std::vector<int> line_no_vec,
                                                ModificationType type);

    /** Returns the path of the file in the DWARF info that most likely fits the cu_file_name
      * Searches by both the full filepath and just the filename
      */
    std::string find_cu_name(std::string &search_path, const char* elf_file_path);

	/* checks if the elf file contains a given source compilation unit */
	bool contains_cu(std::string &cu_name, const char* elf_file_path);

    /* Builds a LineModification from a source file and line number */
    LineModification get_line_modification(std::string src_file_name,
                                           std::string elf_file_path,
                                           int line_num);

    StringPair find_associated_paths(std::string src_file,
                                        std::set<std::string> exe_paths);
};
} //namespace rr

#endif

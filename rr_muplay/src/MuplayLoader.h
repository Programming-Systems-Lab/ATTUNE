#ifndef RR_MUPLAY_LOADER_H
#define RR_MUPLAY_LOADER_H

#include <unordered_map>
#include <string>
#include <elf.h>

#include "ReplaySession.h"
#include "AutoRemoteSyscalls.h"
#include "MuplayElf.h"

namespace rr {

  /* struct to hold info about loaded modified segments */
  struct CustomLoadedSegment {
  public:
    std::string path;
    Elf64_Phdr original_program_header;
    MemoryRange actual_load_addresses;

  };
  /* Loads the modified executable into the target process address space
   * in the future will do some other things like relocating etc. but
   * for now just trying to load the modified executable and maybe change the
   * ip to point to a spot in the new  code based on DWARF info
   */
   class MuplayLoader {
   public:
     /* MuplayElf object holds all information for loading */
     MuplayElf mu_elf;

     /* The target task that the loader puts the code into */
     Task* t;

     /* Constructor with object holding all information from elf
        and the target task */
     MuplayLoader(MuplayElf mu_elf, Task* t);

     /* Finds empty pages in the address space (reads /proc/maps)
        to load the patched code needs to make sure that it loads
        the page within 32 bits of the space where the original code
        is loaded. For now assuming the lowest free pages will do the
        trick */
     std::vector<MemoryRange> find_empty_pages();

     /*
        Load all the loadable segments into the nearest page range
        Returns a list of MemoryRange. In the same order as the
        loadable segments vector in mu_elf.

        This list holds where each loadable segment is stored
        TODO: Turn this into a map instead of a vector
        TODO: For some reason data section not loading with correct permissions
        */
     std::vector<CustomLoadedSegment> load_modified_elf();

     /*
        Given a program header struct load the segment into the nearest
        available page range. Only load into pages that are greater than
        intended virtual address so that relocations can be done simply by adding
        a base address
        TODO base addresses need to be stored somewhere. This probably needs
        to return this information back to the Muplay Session to manage this
       */
     MemoryRange load_into_nearest_page_range(Elf64_Phdr phdr);

     /* Finds the closest memory page range for a given program segment */
     MemoryRange find_closest_page_range(Elf64_Phdr phdr);

     /* macros defined in the mmap library are not the same as the mapping_flags
        from the program header  function converts permissions flag from
        program header to mmap equivalent value */
     /* returns -1 on invalid option */
      int get_mmap_permissions(int p_flags){
        switch(p_flags)
        {
          case 0:
            return 0;
          case 1:
            return PROT_EXEC;
          case 2:
            return PROT_WRITE;
          case 3:
            return PROT_EXEC | PROT_WRITE;
          case 4:
            return PROT_READ;
          case 5:
            return PROT_READ | PROT_EXEC;
          case 6:
            return PROT_READ | PROT_WRITE;
          case 7:
            return PROT_READ | PROT_WRITE | PROT_EXEC;
          default:
            return -1;
        }
      }

   };

} //namespace rr

#endif

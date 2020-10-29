#include <sys/types.h> /* For open() */
#include <sys/stat.h>  /* For open() */
#include <fcntl.h>     /* For open() */
#include <stdlib.h>     /* For exit() */
#include <unistd.h>     /* For close() */
#include <elf.h>

#include "log.h"
#include "ElfReader.h"
#include "MuplayLoader.h"
#include "MemoryRange.h"


using namespace std;

namespace rr {

  MuplayLoader::MuplayLoader(MuplayElf mu_elf, Task* t)
  : mu_elf(mu_elf),
    t(t)
  { }


  std::vector<MemoryRange> MuplayLoader::find_empty_pages()
  {
    /* read the address space */
    std::vector<MemoryRange> res;
    KernelMapping prev;
    bool started = false;
    for (KernelMapIterator it(t); !it.at_end(); ++it)
    {
      KernelMapping km = it.current();
      if(!started)
      {
        started = true;
        prev = km;
        continue;
      }
      int space = km.start().as_int() - prev.end().as_int();

      if(space < 0) {
        //TODO deal with why this is happening. Something with an
        // int long problem
      }
      if(space > 0)
      {
        MemoryRange mem_range(prev.end().as_int(), space);
        res.push_back(mem_range);
      }
      prev = km;
    }
    // Put the end of the final km at the end of the list so that
    // we know we can load at the end of the address space as well

    return res;
  }

  /* Find the closest page range */
  MemoryRange MuplayLoader::find_closest_page_range(Elf64_Phdr phdr)
  {
    MemoryRange res;

    /* Assuming this is in order */
    KernelMapping prev;
    bool started = false;
    Elf64_Addr p_vaddr = phdr.p_vaddr;
    Elf64_Addr p_memsz = phdr.p_memsz;
    for (KernelMapIterator it(t); !it.at_end(); ++it)
    {
      KernelMapping km = it.current();
      if(!started)
      {
        started = true;
        prev = km;
        continue;
      }
      int space = km.start().as_int() - prev.end().as_int();

      if(space < 0) {
        //TODO deal with why this is happening. Something with an
        // int long or signed vs unsigned problem
      }

      //space can accomodate program header and
      // the space is after the original location of the load
      //cast should be safe because of < 0 check above
      if((unsigned int)space > p_memsz && prev.end() > p_vaddr)
      {
        MemoryRange mem_range(prev.end().as_int(), space);
        return mem_range;
      }
      prev = km;
    }

    // If no slots large enough available load the code at
    // the end of the address space
    uint64_t memsz = phdr.p_memsz;
    //DEBUG not sure if this cast is safe
    MemoryRange mem_range(prev.end().as_int(),(size_t) memsz);
    return mem_range;

  }

  std::vector<CustomLoadedSegment> MuplayLoader::load_modified_elf()
  {
      vector<CustomLoadedSegment> res;
      for (Elf64_Phdr segment : mu_elf.elf64_loadable_segments)
      {
        MemoryRange mr(load_into_nearest_page_range(segment));
        CustomLoadedSegment cls;
        cls.path = mu_elf.path;
        cls.original_program_header = segment;
        cls.actual_load_addresses = mr;
        res.push_back(cls);
      }
      return res;
  }
  /* Loading the modified executable segment into the nearest page range
     see note in .h file for more information */
  MemoryRange MuplayLoader::load_into_nearest_page_range(Elf64_Phdr phdr){
    SupportedArch arch = t->arch();
    AutoRemoteSyscalls remote(t);

    // find the closest page range that works
    MemoryRange open_space = find_closest_page_range(phdr);

    //open the file in the tracee process
    AutoRestoreMem child_path(remote, mu_elf.path.c_str());
    int child_fd = remote.syscall(syscall_number_for_open(arch),
                     child_path.get(), O_RDWR);
    if(child_fd < 0)
      FATAL() << "Open failed with errno " << errno_name(-child_fd);

    // load the segment into memory at closest page range
    int mmap_flags = get_mmap_permissions(phdr.p_flags);
    MemoryRange segment_mapping = MemoryRange(remote.infallible_mmap_syscall(open_space.start(),
                                   phdr.p_memsz,
                                   mmap_flags,
                                   MAP_PRIVATE,
                                   child_fd,
                                   /* TODO figure out what offset_pages is */
                                   0), phdr.p_memsz);

    //close the file in the child process
    remote.infallible_syscall(syscall_number_for_close(arch), child_fd);
    return segment_mapping;
  }
}

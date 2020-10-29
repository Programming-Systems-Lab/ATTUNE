#include "log.h"
#include "AutoRemoteSyscalls.h"
#include "MuplayTaskHelper.h"

namespace rr {
MuplayTaskHelper::MuplayTaskHelper(ReplayTask *task) : t(task) {}

    
address_t MuplayTaskHelper::find_lowest_base_address(size_t size) {
    /* Assuming this is in order */
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
            // int long or signed vs unsigned problem
        } else if((unsigned int)space > size) {
            return prev.end().as_int();
        }
        prev = km;
    }

    // If no slots large enough available load the code at
    // the end of the address space
    //DEBUG not sure if this cast is safe
    return prev.end().as_int();
}


address_t MuplayTaskHelper::find_module_base(std::string module_path){
    for(KernelMapIterator it(t); !it.at_end(); ++it) {
        KernelMapping km = it.current();
        // again this only works for the code section
        if(km.fsname() == module_path && km.prot() == (PROT_EXEC | PROT_READ))
            return km.start().as_int();
        else if (get_filename(km.fsname()).find(get_filename(module_path)) != std::string::npos && \
            km.prot() == (PROT_EXEC | PROT_READ))
        return km.start().as_int();
    }
    LOG(warn) << " Could not find module [" + module_path +"] in memory";
    return 0;
}


MemoryRange MuplayTaskHelper::mmap_into_task(address_t address, address_t size,
                                             int mmap_flags, const char* filename)
{
    SupportedArch arch = t->arch();
    AutoRemoteSyscalls remote(t);
    //open the file in the tracee process
    AutoRestoreMem child_path(remote, filename);
    int child_fd = remote.syscall(syscall_number_for_open(arch),
                                  child_path.get(), O_RDWR);
    if (child_fd < 0)
        FATAL() << "Loading patched code failed to open tracee; " << errno_name(-child_fd);
        // load the segment into memory
    MemoryRange segment_mapping = MemoryRange(remote.infallible_mmap_syscall(address,
                                                                             size,
                                                                             mmap_flags,
                                                                             MAP_PRIVATE,
                                                                             child_fd,
                                                                             0), size);
    return segment_mapping;
}


#if 0
std::string MuplayTaskHelper::get_elf_file(unw_word_t mem_address)
{
    /* Returning empty string on process exit */
    if(t == nullptr)
    {
        LOG(warn) << "Process may have exited...get_elf_file returns empty string";
        return "";
    }

    /* Reading the current proc mapping from the AddressSpace */
    /* Need to re init this on each call because mapping changes during replay */
    for (KernelMapIterator it(t); !it.at_end(); ++it)
    {
        KernelMapping km = it.current();
        if (km.contains(mem_address))
        {
            // LOG(debug) << "The mem address 0x" << std::hex << mem_address << " is in file: " << km.fsname();
            return km.fsname();
        }
    }

    LOG(warn) << "Couldn't find /proc entry for mem address: " << std::hex << mem_address;
    /* Returns empty string on not found because the Dwarf Reader checks this */
    return "";

}
#endif

} // namespace rr

/* Defines a few functions that help get information from the current task */
#ifndef RR_MUPLAY_TASK_HELPER_H_
#define RR_MUPLAY_TASK_HELPER_H_

#include "ReplayTask.h"
#include "types.h"

namespace rr {
class MuplayTaskHelper {
public:
    MuplayTaskHelper(ReplayTask* task);

    /* finds the lowest address where there is a slot of unused memory of size size */
    address_t find_lowest_base_address(size_t size);

    /* finds the base address of a given module (filepath) */ 
    address_t find_module_base(std::string module_path);

    /* mmaps in the address space of the task */
    MemoryRange mmap_into_task(address_t address, address_t size,
                               int mmap_flags, const char* filename);
private:
    ReplayTask *t;
};
        
} // namespace rr

#endif

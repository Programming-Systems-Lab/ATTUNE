
/* Statistics that show what was done during a particular mutable replay */
#ifndef RR_MUPLAY_METADATA_H_
#define RR_MUPLAY_METADATA_H_

#include <string>
#include <set>
#include <libdwarf.h>
#include "log.h"

namespace rr {

class MuplayMetadata {
public:
    void dump_metadata();
    void set_output_dir(std::string str) { output_directory = str; }
    void set_modified_code(std::string str) { modified_code = str; }

    void add_ins_address(Dwarf_Addr addr) { 
        LOG(debug) << "adding ins address: " << std::hex << addr; 
        inserted_addresses.insert(addr);
    }

    void add_del_address(Dwarf_Addr addr) { 
        LOG(debug) << "adding del address: " << std::hex << addr; 
        deleted_addresses.insert(addr);
    }

private:
    std::set<Dwarf_Addr> inserted_addresses;
    std::set<Dwarf_Addr> deleted_addresses;

    std::string modified_code;
    std::string output_directory;
};

} // namespace rr

#endif

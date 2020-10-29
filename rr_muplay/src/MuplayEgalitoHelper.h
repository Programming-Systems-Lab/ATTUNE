#ifndef RR_MUPLAY_EGALITO_HELPER_H
#define RR_MUPLAY_EGALITO_HELPER_H

#include "../config/config.h"
#include "conductor/setup.h"
#include "elf/symbol.h"
namespace rr {


#define FITS_IN_ONE_BYTE(x) ((x) < 0x80) /* signed 1-byte operand*/
#define GET_BYTE(x, shift) static_cast<unsigned char> (((x) >> (shift*8)) & 0xff)
#define GET_BYTES(x) GET_BYTE((x), 0), GET_BYTE((x), 1), GET_BYTE((x),2), GET_BYTE((x),3),\
		      GET_BYTE((x), 4), GET_BYTE((x), 5), GET_BYTE((x),6), GET_BYTE((x),7) 

class MuplayEgalitoHelperOptions {
private:
    bool debugMessages = false;
public:
    bool getDebugMessages() const { return debugMessages; }

    void setDebugMessages(bool d) { debugMessages = d; }
};

class MuplayEgalitoHelper {
private:
    MuplayEgalitoHelperOptions options;
    ConductorSetup setup;

    /* Map of register to byte for movabs instruction */
    std::map<std::string, unsigned int> movabsRegsMap {
                                                {"rax", 0x48b8}, 
                                                {"rbx", 0x48bb},
                                                {"rcx", 0x48b9},
                                                {"rdx", 0x48ba},
                                                {"rsi", 0x48be},
                                                {"rdi", 0x48bf},
                                                {"rbp", 0x48bd},
                                                {"rsp", 0x48bc},
                                                {"r8", 0x49b8},
                                                {"r9", 0x49b9},
                                                {"r10", 0x49ba},
                                                {"r11", 0x49bb},
                                                {"r12", 0x49bc},
                                                {"r13", 0x49bd},
                                                {"r14", 0x49be},
                                                {"r15", 0x49bf}
                                           };
public:
    void safe_parse(const char *filename);
    static Symbol *find_symbol(ElfSpace *elfSpace, const char *name);
    MuplayEgalitoHelperOptions &getOptions() { return options; }
    /* Warning! Must delete the ElfSpace that gets created at some point */
    ElfSpace* get_elf_space(std::string elf_path);
    Conductor* get_conductor() { return setup.getConductor(); }
    Module* get_module_from_inst(Instruction *inst);
    /* The set of paths that have been parsed already */
    std::set<std::string> parsed_paths;

    Module* get_parent_module(Chunk *chunk);
    Function* get_parent_function(Chunk *chunk);
    Block* get_parent_block(Chunk* chunk);

    std::string get_data_bytes(DataSection* ds);
    /** Offset is the offset in the region */
    std::string get_ro_data_string(DataRegion* dr, address_t offset);
    std::string get_ro_data_string(DataSection* ds, address_t section_offset);


    address_t get_segment_address(ElfMap* elf_map, ElfSection* section);
    Elf64_Phdr get_phdr(ElfMap* elf_map, ElfSection* section);

    /* Finds the offset of key within the data region*/
    address_t find_string_offset(ElfSpace *elf_space, 
                                 const std::string &key, 
                                 std::string &section);

    Symbol* lookup_symbol(ElfSpace *elf_space, std::string symbol_name);
    address_t find_module_offset(std::string module_path, int flags);

    void rewrite_plt_entry(PLTTrampoline *tramp, address_t target_address);
    void lea_to_movabs(Instruction* ins, address_t target);
};
} // namespace rr

#endif

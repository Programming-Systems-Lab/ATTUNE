#define DEBUG_GROUP chunk

#include <unistd.h>
#include <iostream>
#include <functional>
#include <cstring>  // for std::strcmp
#include "MuplayEgalitoHelper.h"
#include "log.h"

#include "conductor/conductor.h"
#include "chunk/dump.h"
#include "pass/chunkpass.h"
#include "log/registry.h"
#include "log/temp.h"
#include "pass/clearplts.h"
#include "disasm/disassemble.h"
#include "operation/mutator.h"
//#include "log/log.h"

namespace rr {

void MuplayEgalitoHelper::safe_parse(const char *filename) {
  
    std::cout << "Parsing: [" << filename << "]\n";
    if(parsed_paths.find(filename) == parsed_paths.end())
        parsed_paths.insert(filename);
    else {
        std::cout << "ALREADY PARSED: [" << filename << "]\n";
        return;
    }

    if(!SettingsParser().parseEnvVar("EGALITO_DEBUG")) { /*Throw error here*/}

    if(!options.getDebugMessages()) {
        // Note: once we disable messages, we never re-enable them.
        // Right now the old settings aren't saved so it's not easy to do.
        GroupRegistry::getInstance()->muteAllSettings();
    }

    try {
        if(ElfMap::isElf(filename)) {
            const bool recursive = true;
            const bool includeEgalitoLib = false;
            setup.parseElfFiles(filename, recursive, includeEgalitoLib);
        }
        else {
            setup.parseEgalitoArchive(filename);
        }
    }
    catch(const char *message) {
        std::cout << "Exception: " << message << std::endl;
    }
}

Symbol* MuplayEgalitoHelper::find_symbol(ElfSpace *elfSpace, const char *name) {
    auto symbolList = elfSpace->getSymbolList();
    return symbolList ? symbolList->find(name) : nullptr;
}

ElfSpace* MuplayEgalitoHelper::get_elf_space(std::string elf_path) {
    if(elf_path == "") return nullptr;
    
    ElfMap *elf_map = new ElfMap(elf_path.c_str());
    ElfSpace *elf_space = new ElfSpace(elf_map, elf_path, elf_path);
    elf_space->findSymbolsAndRelocs();
    return elf_space;
}

Block* MuplayEgalitoHelper::get_parent_block(Chunk* chunk) {
    Block* res = nullptr;
    while (!res) {
        chunk = chunk->getParent();
        res = dynamic_cast<Block*>(chunk);
        if(!chunk) return nullptr;
    }
    return res;
}

Module* MuplayEgalitoHelper::get_parent_module(Chunk *chunk) {
    Module *res = nullptr;
    while(!res) {
        chunk = chunk->getParent();
        res = dynamic_cast<Module*>(chunk);

        if(!chunk) return nullptr;
    }
    return res;
}

Function* MuplayEgalitoHelper::get_parent_function(Chunk *chunk) {
    Function *res = nullptr;
    while(!res) {
        chunk = chunk->getParent();
        res = dynamic_cast<Function*>(chunk);

        if(!chunk) return nullptr;
    }
    return res;
}


std::string MuplayEgalitoHelper::get_data_bytes(DataSection* ds) {
    DataRegion* dr = dynamic_cast<DataRegion*>(ds->getParent());
    std::string data_bytes = dr->getDataBytes();
    return data_bytes;
   // return dynamic_cast<DataRegion*>(ds->getParent())->getDataBytes();
}

std::string MuplayEgalitoHelper::get_ro_data_string(DataRegion* dr, address_t region_offset) { 
    //LOG(debug) << "Looking for string at offset: " << std::hex << region_offset;
    auto data_bytes = dr->getDataBytes().data();
    std::string data_string(data_bytes + region_offset);
    //LOG(debug) << "Found string: [" << data_string << "]"
               //<< " at " << std::hex << region_offset ;
    return data_string;
}

std::string MuplayEgalitoHelper::get_ro_data_string(DataSection* ds, address_t section_offset) {
    if(ds) { }
    if(section_offset) { }
    return "";
}

void MuplayEgalitoHelper::rewrite_plt_entry(PLTTrampoline *tramp, address_t target_address) {
	//Clear the existing PLT Blocks
	ClearPLTs clear_plts;	
	tramp->accept(&clear_plts);
	// 0: 49 bb ef be ad de ef 	movabs r11, 0xdeadbeefdeadbeef
	// 7: be ad de
	// a: 41 ff e3			jmp r11
	
	Instruction *mov_abs_instr = Disassemble::instruction({0x49, 0xbb, GET_BYTES(target_address)});
	Instruction *jmp_instr = Disassemble::instruction({0x41, 0xff, 0xe3});
	std::vector<Instruction*>{mov_abs_instr, jmp_instr};

	PositionFactory *position_factory = PositionFactory::getInstance();
	Block *jmp_block = new Block();
	jmp_block->setPosition(position_factory ->makePosition(nullptr, jmp_block, 0));

	ChunkMutator mu(jmp_block);
	mu.append(mov_abs_instr);
	mu.append(jmp_instr);
	ChunkMutator mu2(tramp, true);
    mu2.append(jmp_block);
}

address_t MuplayEgalitoHelper::get_segment_address(ElfMap* elf_map, ElfSection* section) {
    auto segmentList = elf_map->getSegmentList();
    auto shdr = section->getHeader();

    for(void *p : segmentList) {
        auto phdr = static_cast<Elf64_Phdr *>(p);
        auto low_addr = phdr->p_vaddr;
        auto high_addr = low_addr + phdr->p_memsz;
        if( shdr->sh_addr >= low_addr && shdr->sh_addr <= high_addr) return phdr->p_vaddr;
    }
    FATAL() << "Couldn't find associated segment for section " << section->getName();
    return 0;
}

Elf64_Phdr MuplayEgalitoHelper::get_phdr(ElfMap* elf_map, ElfSection* section) { 
    auto segment_list = elf_map->getSegmentList();
    auto shdr = section->getHeader();

    for(void *p : segment_list) {
        auto phdr = static_cast<Elf64_Phdr *>(p);

        auto low_addr = phdr->p_vaddr;
        auto high_addr = low_addr + phdr->p_memsz;
        if( shdr->sh_addr >= low_addr && shdr->sh_addr <= high_addr) return *phdr;
    }
    FATAL() << "Couldn't find associated segment for section " << section->getName();
    Elf64_Phdr fake;
    return fake;
}

address_t MuplayEgalitoHelper::find_string_offset(ElfSpace* elf_space, const std::string &key, 
                                                  std::string &section) {
    if(section != ".rodata") FATAL() << "Trying to find string not in .rodata"; // Not needed. Curious if ever happens
#if 0
    ElfMap *elf_map = elf_space->getElfMap();
    ElfSection* elf_section = elf_map->findSection(section.c_str());
    Elf64_Phdr phdr = get_phdr(elf_map, elf_section);
    DataRegion dr(elf_map, &phdr);

    address_t offset = 0;
    while(offset < dr.getSize()) {
       auto string = get_ro_data_string(&dr, offset);
       if(string == key) return offset;
       
       offset += string.length()==0 ? 1 : string.length();
    }
    LOG(debug) << "Couldn't find string: " << key << " in " << elf_space->getName();
    return -1;
    /* Need to make a DataRegion here  -- find Phdr*/ 
 #endif   

    ElfMap *elf_map = elf_space->getElfMap();
    ElfSection* elf_section = elf_map->findSection(section.c_str());
    address_t start_read_ptr = elf_section->getReadAddress();
    char* read_ptr = (char*) start_read_ptr;
    char* end_ptr = (char*) start_read_ptr + elf_section->getSize();
    char* res = 0;
    while(read_ptr <= end_ptr) {
        if(!strcmp(read_ptr, key.c_str())){
            LOG(debug) << "Found string offset for string [" << std::string(read_ptr) << "]";
            res = read_ptr;
            break;
        }
        read_ptr += strlen(read_ptr) + 1;
    }
    if(!res) {
        LOG(debug) << "String not found: " << key;
        return -1;
    }
    // want the offset into the section not the actual address
    return (address_t) res - start_read_ptr;
}

Symbol* MuplayEgalitoHelper::lookup_symbol(ElfSpace *elf_space, std::string symbol_name){
    auto symbol_list = elf_space->getSymbolList();
    return symbol_list ? symbol_list->find(symbol_name.c_str()) : nullptr;
}

// Rewrite the lea instruction as a movabs instruction
void MuplayEgalitoHelper::lea_to_movabs(Instruction* ins, address_t target) {
    Assembly* assm = ins->getSemantic()->getAssembly().get();
    std::string mnemonic = assm->getMnemonic();
    std::string opStr = assm->getOpStr();
    int movabs_opcode = 0;
    for(auto it = movabsRegsMap.begin(); it != movabsRegsMap.end(); it++)
    {
      if (opStr.find(it->first) != std::string::npos) {
          movabs_opcode = it->second;
          break;
      }
    }
    unsigned char b0 = GET_BYTE(movabs_opcode, 1);
    unsigned char b1 = GET_BYTE(movabs_opcode, 0);

    Block *block = get_parent_block(ins);
    if(!block) FATAL() << "Couldn't find parent block for [" << ins << "]";

    Instruction *mov_abs_ins = Disassemble::instruction({b0, b1, GET_BYTES(target)});
    ChunkMutator(block).insertAfter(ins, mov_abs_ins);

    // TODO Remove the now useless instruction      

    //for(int i = block->getChildren()->genericGetSize() - 1; i >= 0; i --) {
    //  auto child = block->getChildren()->genericGetAt(i);
    //  if(dynamic_cast<Instruction*>(child) == inst)
    //      delete child;
    //}
}

} // namespace rr

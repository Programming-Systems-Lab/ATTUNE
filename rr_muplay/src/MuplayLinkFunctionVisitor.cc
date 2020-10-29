#include "log.h"
#include "AddressSpace.h"
#include "MuplayLinkFunctionVisitor.h"
#include "MuplayOriginalSymbolLink.h"
#include "MuplayEgalitoHelper.h"
#include "MuplayStatistics.h"
#include "util.h"

namespace rr {

MuplayLinkFunctionVisitor::MuplayLinkFunctionVisitor(std::set<std::string> &symbols_to_resolve,
                                  std::map<Instruction*, GlobalVarDataRef> &globals_to_resolve,
                                  std::map<Instruction*, StringDataRef> &strings_to_resolve,
                                  ElfSpace *old_elf_space, Task* t, MuplayStatistics* stats) :
    symbols_to_resolve(symbols_to_resolve),
    globals_to_resolve(globals_to_resolve),
    strings_to_resolve(strings_to_resolve),
    old_elf_space(old_elf_space),
    t(t), stats(stats) { }


void MuplayLinkFunctionVisitor::visit(Function *function) {
    LOG(debug) << "Linking function: " << function->getName() << "\n";
    recurse(function);
}

void MuplayLinkFunctionVisitor::visit(Block *block) {
   recurse(block);
}

void MuplayLinkFunctionVisitor::visit(Instruction *inst) {
    auto semantic = inst->getSemantic();
    if(auto link = semantic->getLink()) {
        auto target_chunk = link->getTarget();
        if(!target_chunk) {
            LOG(debug) << "Target chunk not found for " << inst->getName();
            return;
        }

        if(auto data_offset_link = dynamic_cast<DataOffsetLink*>(link)) {
            link_data_offset_link(inst, data_offset_link);
            stats->increment_num_data_references_resolved();
        } else if (auto absolute_data_link = dynamic_cast<AbsoluteDataLink*>(link)) {
            if(absolute_data_link) FATAL() << "Can't link absolute data link";
            stats->increment_num_data_references_resolved();
        } else if(auto target = dynamic_cast<Function*>(target_chunk)) {
            link_function_target(inst, target);
            stats->increment_num_code_references_resolved();
        } else if (Instruction *target = dynamic_cast<Instruction*>(target_chunk)) {
            link_instruction_target(inst, target);
            stats->increment_num_code_references_resolved();
        } else if (PLTTrampoline *target = dynamic_cast<PLTTrampoline*>(target_chunk)) {
            link_plt_target(target);
            stats->increment_num_code_references_resolved();
            stats->increment_num_plt_references_resolved();
        } else {
            // Other links that need to be resolved
            LOG(debug) << "Unhandled Link target " << target_chunk->getName();
        }
        
    }
}


void MuplayLinkFunctionVisitor::link_plt_target(PLTTrampoline *target) {
    LOG(debug)<<"Resolving PLT Reference to: " << target->getName();
    
    ExternalSymbol *extsym = target->getExternalSymbol();
    Module *module = extsym->getResolvedModule();
    std::string module_path = real_path(module->getElfSpace()->getFullPath());
    LOG(debug) << "Target module of [" << target->getName() << "] is [" << module_path << "]";
    
    Function *target_function = dynamic_cast<Function *>(extsym->getResolved());
    if(!target_function) FATAL() << "PLT Trampoline doesn't resolved to function";

    address_t func_offset = target_function->getAddress();
    address_t module_offset = find_module_offset(module_path, (PROT_EXEC | PROT_READ));

    if(module_offset == 0) {
      LOG(warn) << "Module: [" + module_path + "] not loaded";
      LOG(warn) << "PLT Reference UNLINKED!!!!!";
      return;
    }

    address_t oldPLTTargetAddress = module_offset + func_offset;
    MuplayEgalitoHelper().rewrite_plt_entry(target, oldPLTTargetAddress);
}

void MuplayLinkFunctionVisitor::link_instruction_target(Instruction *src, Instruction *target) {
// TODO THIS NEEDS DEBUGGING
    LOG(debug) << "Linking Instruction Reference from [" << src->getName() << "] to ["  << target->getName() << "]";
    if(Function *parent_function = MuplayEgalitoHelper().get_parent_function(src))
    {
        //create a link for this instruction in the old binary
        // position is defined as offset from parent function
        Symbol* old_symbol = MuplayEgalitoHelper().lookup_symbol(old_elf_space, parent_function->getName());
        
        if(!old_symbol) return;
        address_t offset = target->getAddress() - parent_function->getAddress();
        address_t targetAddress = old_symbol->getAddress()+offset;
        //Original Symbol Code Link
        src->getSemantic()->setLink(new MuplayOriginalSymbolLinkText(old_symbol, targetAddress, offset));
    } else
        FATAL()<<"Couldn't find parent function of source instruction [" << src->getName() << "]";
    
}

void MuplayLinkFunctionVisitor::link_function_target(Instruction *src, Function *target) {
    auto target_name = target->getName();
    LOG(debug) << "Linking Function Reference from [" << src->getName() << "] to [" << target_name << "]"; 

    Symbol* old_symbol = MuplayEgalitoHelper().lookup_symbol(old_elf_space, target_name);
    if(!old_symbol) return;
    address_t target_address = old_symbol->getAddress();
    address_t offset = 0;
    LOG(debug) << "Address of [" << target_name << "] in original binary: " << target_address << " \n";
    if(src->getSemantic()->getLink()->isRIPRelative())
    {
        //Original Symbol Code Link
        src->getSemantic()->setLink(new MuplayOriginalSymbolLinkText(old_symbol, target_address, offset));
    } else {
        // Original symbol data link
        src->getSemantic()->setLink(new MuplayOriginalSymbolLinkData(old_symbol, target_address, offset));
    }

}

void MuplayLinkFunctionVisitor::link_data_offset_link(Instruction *inst, DataOffsetLink *link) { 
    auto string_ref = strings_to_resolve.find(inst);
    auto global_ref = globals_to_resolve.find(inst);
    if(string_ref != strings_to_resolve.end() 
       && string_ref->second.location == DataRefLocation::OLD_EXE) {
        link_string_reference(inst, link);
    } else if (global_ref != globals_to_resolve.end()
               && string_ref->second.location == DataRefLocation::OLD_EXE) {
        link_global_variable_reference(inst, link);
    } else if (string_ref != strings_to_resolve.end() && 
               string_ref->second.location == DataRefLocation::NEW_EXE){
        LOG(debug) << "Need to link instruction to new string at: " << inst;
    } else {
        return;
        FATAL() << "Couldn't link data offset link instruction: " << inst;
    }
}

void MuplayLinkFunctionVisitor::link_global_variable_reference(Instruction *inst, DataOffsetLink *link) {
    if(link) {}
    LOG(warn) << "Unhandled global variable reference at: " << inst;
    /* get the symbol in question */
    /* search the symbol in the old binary */
    /* make transformation to reference the new variable */
}

void MuplayLinkFunctionVisitor::link_string_reference(Instruction *inst, DataOffsetLink *link) {
    if(strings_to_resolve.find(inst) == strings_to_resolve.end()) return;
    LOG(debug) << "Resolving string reference at: " << std::hex << inst->getAddress();

    auto target_name = link->getTarget()->getName(); 
    auto key = strings_to_resolve.at(inst).string;
    address_t string_offset = MuplayEgalitoHelper().find_string_offset(old_elf_space, key, target_name);
    if((int)string_offset < 0) {
        FATAL() << "Couldn't link string: " << key  << " to original executable";
    }

    // Find the address where that string is loaded in the recorded context
    DataSection* data_section = dynamic_cast<DataSection*> (link->getTarget());
    DataRegion*  data_region = dynamic_cast<DataRegion*> (data_section->getParent());
    int permissions = 0;
    if(data_region->readable()) permissions = (permissions | PROT_READ);
    if(data_region->writable()) permissions = (permissions | PROT_WRITE);
    if(data_region->executable()) permissions = (permissions | PROT_EXEC);
    
    address_t module_offset = find_module_offset(old_elf_space->getFullPath(), permissions);

    address_t string_address = module_offset + string_offset;
    LOG(debug) << "Found string: [" << key << "] at 0x" << std::hex << string_address;

    const char* bytes = inst->getSemantic()->getAssembly()->getBytes();
    //check if lea 0xoffset(%rip), reg instruction
    //NOTE this may not be the right way to check this
    if (bytes[0] == 0x48 && bytes[1] == (char) 0x8d) {
        MuplayEgalitoHelper().lea_to_movabs(inst, string_address);
    }
    LOG(debug) << "Updated String reference to: 0x" << std::hex << string_address; 

    //semantic->setLink(new MuplayOriginalSymbolLinkText(nullptr, stringAddress, 0));
}

address_t MuplayLinkFunctionVisitor::find_module_offset(std::string module_path, int flags) {
    address_t module_offset = 0;
    for (KernelMapIterator it(t); !it.at_end(); ++it)
    {
        KernelMapping km = it.current();
        /* TODO include check for rr special files */
        if(km.fsname() == module_path && km.prot() == flags) {
            module_offset = km.start().as_int();
            break;
        }
        else if (get_filename(km.fsname()).find(get_filename(module_path)) != std::string::npos  && \
                 km.prot() == flags) {
            module_offset = km.start().as_int();
            break;         
        }
    }
    if(!module_offset) return 0;
    return module_offset;
}


#if 0
#define FITS_IN_ONE_BYTE(x) ((x) < 0x80) /* signed 1-byte operand*/
#define GET_BYTE(x, shift) static_cast<unsigned char> (((x) >> (shift*8)) & 0xff)
#define GET_BYTES(x) GET_BYTE((x), 0), GET_BYTE((x), 1), GET_BYTE((x),2), GET_BYTE((x),3),\
		      GET_BYTE((x), 4), GET_BYTE((x), 5), GET_BYTE((x),6), GET_BYTE((x),7) 
#endif 

#if 0
void MuplayLinkFunctionVisitor::rewritePLTEntry(PLTTrampoline *tramp, address_t targetAddress) {
	//Clear the existing PLT Blocks
	ClearPLTs clearPLTs;	
	tramp->accept(&clearPLTs);
	// 0: 49 bb ef be ad de ef 	movabs r11, 0xdeadbeefdeadbeef
	// 7: be ad de
	// a: 41 ff e3			jmp r11
	
	Instruction *movAbsInstr = Disassemble::instruction({0x49, 0xbb, GET_BYTES(targetAddress)});
	Instruction *jmpInstr = Disassemble::instruction({0x41, 0xff, 0xe3});
	std::vector<Instruction*>{movAbsInstr, jmpInstr};

	PositionFactory *positionFactory = PositionFactory::getInstance();
	Block *jmpBlock = new Block();
	jmpBlock->setPosition(positionFactory->makePosition(nullptr, jmpBlock, 0));

	ChunkMutator mu(jmpBlock);
	mu.append(movAbsInstr);
	mu.append(jmpInstr);
	ChunkMutator mu2(tramp, true);
	mu2.append(jmpBlock);
  }
#endif

#if 0
  address_t MuplayLinkFunctionVisitor::findStringOffset(std::string string, std::string section) {
    //Finds the address of a string in the old elf space
    ElfSection* elfSection = old_elf_space->getElfMap()->findSection(section.c_str());
    address_t startReadPtr = elfSection->getReadAddress();
    char* readPtr = (char*) startReadPtr;
    char* endPtr = (char*) startReadPtr + elfSection->getSize();
    char* res = 0;
    /* printf("String 1: %s \n",(char*) startReadPtr);
    char* readPtr = (char*) startReadPtr;
    char* readPtr2 = readPtr + strlen(readPtr) + 1;
    printf("String 2: %s \n", readPtr2);
    */  
    while(readPtr <= endPtr) {
        if(!strcmp(readPtr, string.c_str())){
            LOG(debug) << "Found string; " << std::string(readPtr);
            res = readPtr;
            break;
        }
        readPtr+=strlen(readPtr) + 1;
    }
    // THIS NEEDS MATH DONE TO IT
    if(!res) {
        LOG(debug) << "String not found: " << string;
        return 0;
    }
    // want the offset into the section not the actual address
    return (address_t) res - startReadPtr;
    
  }
#endif   

  #if 0
  // Rewrite the lea instruction as a movabs instruction
  void MuplayLinkFunctionVisitor::rewriteLeaInstruction(Instruction* inst, address_t target) {
      Assembly* assm = inst->getSemantic()->getAssembly().get();
      std::string mnemonic = assm->getMnemonic();
      std::string opStr = assm->getOpStr();
      int movAbsOpCode = 0;
      for(auto it = movabsRegsMap.begin(); it != movabsRegsMap.end(); it++)
      {
          if (opStr.find(it->first) != std::string::npos) {
              movAbsOpCode = it->second;
              break;
          }
      }
      unsigned char b0 = GET_BYTE(movAbsOpCode, 1);
      unsigned char b1 = GET_BYTE(movAbsOpCode, 0);
      if(inst) { } 
      if(target){ }
      if(assm) { }
      if(mnemonic.size()) { } 
      if(opStr.size()) { } 
      
      Block *block = dynamic_cast<Block*>(inst->getParent());
	  Instruction *movAbsInstr = Disassemble::instruction({b0, b1, GET_BYTES(target)});
      ChunkMutator(block).insertAfter(inst, movAbsInstr);
      // TODO Remove the now useless instruction      

      //for(int i = block->getChildren()->genericGetSize() - 1; i >= 0; i --) {
      //  auto child = block->getChildren()->genericGetAt(i);
      //  if(dynamic_cast<Instruction*>(child) == inst)
      //      delete child;
      //}
  }
  #endif
} // namespace rr


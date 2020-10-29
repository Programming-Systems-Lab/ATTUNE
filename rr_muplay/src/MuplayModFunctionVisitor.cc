#include "MuplayModFunctionVisitor.h"
#include "MuplayEgalitoHelper.h"
#include "log.h"
#include "elf/symbol.h"

namespace rr {
void MuplayModFunctionVisitor::visit(Function *function) {
    LOG(debug) << "ModFunctionVisitor running on function: [" 
               << function->getName() << "]\n";
    recurse(function);
}

void MuplayModFunctionVisitor::visit(Block *block) {
    recurse(block);
}

void MuplayModFunctionVisitor::visit(Instruction *inst) {
    auto semantic = inst->getSemantic();
    if(auto link = semantic -> getLink()) {
        ChunkRef target_chunk = link -> getTarget();

        if(auto offset_data_link = dynamic_cast<DataOffsetLink*>(link)) {
            handle_offset_data_link(inst, offset_data_link);
        } else if(auto absolute_data_link = dynamic_cast<AbsoluteDataLink*>(link)) {
            LOG(warn) << "Unhandled absolute data link\n";
            if(absolute_data_link) {}
        } else if(Function *target_function = dynamic_cast<Function*>(target_chunk)) {
            handle_function_link(target_function);
        } else if (Instruction *target = dynamic_cast<Instruction*>(target_chunk)) {
            handle_instruction_link(target);
        } else if (PLTTrampoline *tramp = dynamic_cast<PLTTrampoline*>(target_chunk)) {
            symbols_to_resolve.insert(tramp->getName()); 
        } else {
            LOG(warn) << "Unknown link from [" << inst << "] to [" << target_chunk << "]";
        }
    }
}

void MuplayModFunctionVisitor::handle_offset_data_link(Instruction* src, DataOffsetLink *offset_data_link) {
    auto data_section = dynamic_cast<DataSection *>(offset_data_link->getTarget());
    LOG(debug)<<"Data Region reference to [" << data_section->getName() 
              << "] at: " << src->getName() << "\n";

    Symbol* glob_sym = nullptr;

    std::vector<GlobalVariable *>globals = data_section->getGlobalVariables();
    for (auto global : globals) {
        Module *module = MuplayEgalitoHelper().get_parent_module(src);
        if(!module) FATAL() << "Couldn't find module for instruction: " << src;

        if(global->getSymbol()->getAddress() 
           == offset_data_link->getTargetAddress() - module->getBaseAddress()){
            LOG(debug) << "Global variable reference: " << global->getName();
            glob_sym = global->getSymbol(); 
            GlobalVarDataRef gvdr(glob_sym, DataRefLocation::OLD_EXE);
            globals_to_resolve.insert({src, gvdr});
            break;
        }
    }
    if(!glob_sym && data_section->getName() == ".rodata") {
        // assume this is a string
        auto data_region = dynamic_cast<DataRegion*>(data_section->getParent());
        address_t region_offset = offset_data_link->getTargetAddress()
                                  - data_region->getAddress();
        std::string data_string = MuplayEgalitoHelper().get_ro_data_string(data_region,
                                                                           region_offset);

        StringDataRef sdr(data_string, DataRefLocation::OLD_EXE);
        strings_to_resolve.insert({src, sdr});
        LOG(debug) << "Found string reference  [" << src << "] to [" << data_string << "]";
    }    

}

void MuplayModFunctionVisitor::handle_instruction_link(Instruction *target) {
      Function *func = MuplayEgalitoHelper().get_parent_function(target);
      if(func)
        symbols_to_resolve.insert(func->getName());
      else
        FATAL()<<"ModFunctionVisitor couldn't find parent function of: " << target->getName();
}

void MuplayModFunctionVisitor::handle_function_link(Function *target) {
    symbols_to_resolve.insert(target->getName());
}

void MuplayModFunctionVisitor::handle_plt_link(PLTTrampoline *target) {
    symbols_to_resolve.insert(target->getName()); 
}


  
//need to clear the plt entries so that they can be repopulated
void MuplayModFunctionVisitor::visit(PLTTrampoline *plt) {
    freeChildren(plt, 2); 
}

void MuplayModFunctionVisitor::freeChildren(Chunk *chunk, int level) {
    if(level > 0) {
        for(int i = chunk->getChildren()->genericGetSize() - 1; i>=0; i--) {
            auto child = chunk->getChildren()->genericGetAt(i);
			freeChildren(child, level-1);
			chunk->getChildren()->genericRemoveLast();
			delete child;
        }
    }
}


} //namespace rr

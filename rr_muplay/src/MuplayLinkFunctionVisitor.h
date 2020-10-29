#ifndef RR_MUPLAY_LINK_FUNCTION_VISITOR_
#define RR_MUPLAY_LINK_FUNCTION_VISITOR_

#include "chunk/chunk.h"
#include "chunk/concrete.h"
#include "instr/visitor.h"
#include "instr/concrete.h"
#include "chunk/visitor.h"
#include "elf/elfspace.h"
#include "elf/symbol.h"

#include "Task.h"
#include "MuplayTypes.h"
#include "MuplayStatistics.h"

namespace rr {
  class MuplayLinkFunctionVisitor : public ChunkVisitor {
  private:
    // TODO move this to some other .h file that both visitors call
      template <typename Type>
      void recurse(Type *root) {
          for(auto child : root->getChildren()->genericIterable()) {
              //if(child->getName() == "operate_do/bb+1507") return;
              if(child->getName() == "Curl_follow/bb+813") return;
              if(child->getName() == "read_file/bb+66") return;

              if(!child) return;
              child->accept(this);
          }
      }
      std::set<std::string> symbols_to_resolve;
      std::map<Instruction*, GlobalVarDataRef> globals_to_resolve;
      std::map<Instruction*, StringDataRef> strings_to_resolve; 
      ElfSpace *old_elf_space;
	  /* Constructor for the class */
	  MuplayLinkFunctionVisitor() = default;

    /* Function to resolve the target chunk in the old space and make the link properly */
    void updateLink(Link *targetName);

    /* The task that the code is being linked into */
    Task* t;

  public:
    /* Constructor includes ElfSpace of Original File for lookups
       and the symbols that need to be resolved */
    MuplayLinkFunctionVisitor(std::set<std::string> &symbols_to_resolve,
		   	      std::map<Instruction*, GlobalVarDataRef> &globals_to_resolve,
			      std::map<Instruction*, StringDataRef> &strings_to_resolve,
		   	      ElfSpace *old_elf_space, Task* t, MuplayStatistics* stats);
    /* specific to this visitor */
    virtual void visit(Function *function);
    virtual void visit(Block *instr);
    virtual void visit(Instruction *instr);
    /* specific to this visitor */

    virtual void visit(Program *program) { recurse(program); }
    virtual void visit(Module *module) { recurse(module); };
    virtual void visit(FunctionList *functionList) { recurse(functionList); };
    virtual void visit(PLTList *pltList) { recurse(pltList); };
    virtual void visit(JumpTableList *jumpTableList) { recurse(jumpTableList); };
    virtual void visit(DataRegionList *dataRegionList) { recurse(dataRegionList); };
    virtual void visit(VTableList *vtable) { recurse(vtable); };
    virtual void visit(InitFunctionList *initFunctionList) { recurse(initFunctionList); };
    virtual void visit(ExternalSymbolList *externalSymbolList) { recurse(externalSymbolList); };
    virtual void visit(LibraryList *libraryList) { recurse(libraryList); };

    virtual void visit(PLTTrampoline *trampoline) { recurse(trampoline); };
    virtual void visit(JumpTable *jumpTable) { recurse(jumpTable); };
    virtual void visit(JumpTableEntry *jumpTableEntry) { recurse(jumpTableEntry); };
    virtual void visit(DataRegion *dataRegion) { recurse(dataRegion); };
    virtual void visit(DataSection *dataSection) { recurse(dataSection); };
    virtual void visit(DataVariable *dataVariable) { recurse(dataVariable); };
    virtual void visit(GlobalVariable *globalVariable) { recurse(globalVariable); };
    virtual void visit(MarkerList *markerList) { recurse(markerList); };
    virtual void visit(VTable *vtable) { recurse(vtable); };
    virtual void visit(VTableEntry *vtableEntry) { recurse(vtableEntry); };
    virtual void visit(InitFunction *initFunction) { recurse(initFunction); };
    virtual void visit(ExternalSymbol *externalSymbol) { recurse(externalSymbol); };
    virtual void visit(Library *library) { recurse(library); };

    std::set<std::string> getSymbolsToResolve() { return symbols_to_resolve; }
    std::map<Instruction*, GlobalVarDataRef> getGlobalsToResolve() { return globals_to_resolve; }
    std::map<Instruction*, StringDataRef> getStringsToResolve() { return strings_to_resolve; }
    MuplayStatistics* stats;

    void link_plt_target(PLTTrampoline *target);
    void link_instruction_target(Instruction *src, Instruction *target);
    void link_function_target(Instruction *src, Function *target);
    void link_data_offset_link(Instruction *inst, DataOffsetLink *link);
    void link_global_variable_reference(Instruction *inst, DataOffsetLink *link);
    void link_string_reference(Instruction *inst, DataOffsetLink *link);

    // Finds the offset of the given module path in the replaying task context
    // Searches for the module with the given permissions flags
    address_t find_module_offset(std::string module_path, int flags);




}; 
  
  
}//namespace rr

#endif

#ifndef RR_MUPLAY_MOD_FUNCTION_VISITOR_
#define RR_MUPLAY_MOD_FUNCTION_VISITOR_

#include "chunk/chunk.h"
#include "chunk/concrete.h"
#include "instr/visitor.h"
#include "instr/concrete.h"
#include "chunk/visitor.h"
#include "elf/elfspace.h"
#include "elf/symbol.h"
#include "MuplayTypes.h"


namespace rr {
  class MuplayModFunctionVisitor : public ChunkVisitor {
  private:
      // TODO move this to some other .h file that both visitors call
      template <typename Type>
      void recurse(Type *root) {
          for(auto child : root->getChildren()->genericIterable()) {
              child->accept(this);
          }
      }
      // ElfSpace *origSpace;
      // TODO turn symbols to Resolve into functionsToResolve
      std::set<std::string> symbols_to_resolve;
      std::map<Instruction*, GlobalVarDataRef> globals_to_resolve;
      std::map<Instruction*, StringDataRef> strings_to_resolve;
  public:
    /* Constructor includes ElfSpace of Original File for lookups */
    // FunctionVisitor(ElfSpace *origSpace);
    /* specific to this visitor */
    virtual void visit(Function *function);
    virtual void visit(Block *instr);
    virtual void visit(Instruction *instr);
    virtual void visit(PLTTrampoline *trampoline);
    /* specific to this visitor */

    virtual void visit(Program *program) { recurse(program); };
    virtual void visit(Module *module) { recurse(module);
    					 recurse(module->getPLTList()); };
    virtual void visit(FunctionList *functionList) { recurse(functionList); };
    virtual void visit(PLTList *pltList) { recurse(pltList); };
    virtual void visit(JumpTableList *jumpTableList) { recurse(jumpTableList); };
    virtual void visit(DataRegionList *dataRegionList) { recurse(dataRegionList); };
    virtual void visit(VTableList *vtable) { recurse(vtable); };
    virtual void visit(InitFunctionList *initFunctionList) { recurse(initFunctionList); };
    virtual void visit(ExternalSymbolList *externalSymbolList) { recurse(externalSymbolList); };
    virtual void visit(LibraryList *libraryList) { recurse(libraryList); };

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

    /* Additional Helper Functions that should probably be moved to another class */
    Symbol* lookupSymbol(std::string symbolName);

    std::set<std::string> get_symbols_to_resolve() { return symbols_to_resolve; }
    std::map<Instruction*, GlobalVarDataRef> get_globals_to_resolve() { return globals_to_resolve; }
    std::map<Instruction*, StringDataRef> get_strings_to_resolve() { return strings_to_resolve; }
    
    void freeChildren(Chunk *chunk, int level);

    void handle_offset_data_link(Instruction *src, DataOffsetLink *offset_data_link);
    void handle_instruction_link(Instruction *target);
    void handle_function_link(Function *target);
    void handle_plt_link(PLTTrampoline *target);
  };
}
#endif

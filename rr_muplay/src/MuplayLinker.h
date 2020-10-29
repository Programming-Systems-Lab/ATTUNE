#ifndef RR_MUPLAY_LINKER_H_
#define RR_MUPLAY_LINKER_H_

#include <memory>
#include "MuplayElf.h"
#include "MuplaySession.h"

namespace rr {


  /* Some functions can read either the old or the modified elf. This is a
     flag that specifies which one you want */
  enum MUPLAY_READER_TYPE { OLD, MOD };


  /* Once the new code has been loaded need to link it in the
     context of the original executable. This means both
     the relocation entries and the global variable references */

  class MuplayLinker{
  public:
    /* Constructors */
    MuplayLinker(MuplayElf& old_elf,
                 MuplayElf& mod_elf,
                 std::vector<CustomLoadedSegment>& custom_segments,
                 std::vector<MuplayBinaryModificationSummary>& summaries,
                 Task* curr);

    /* Fields */
    MuplayElf old_elf; /* The exe that is recorded */
    MuplayElf mod_elf; /* exe with the modifications */
    std::vector<CustomLoadedSegment> custom_segments;
    std::vector<MuplayBinaryModificationSummary> summaries;
    Task* curr; /*Task that is being traced and will be overwritten */

    MuplayElfReader old_elf_reader;
    MuplayElfReader mod_elf_reader;

    /* Functions */

    /* Do the standard relocations as required by the modified elf file */
    void do_standard_relocations();

    /* Do a single standard relocation */
    void do_standard_relocation(Elf64_Rela rela, Elf64_Shdr rel_shdr);

    /* Do all the relocations required by the patched file */
    void do_all_relocations();

    /* gets the symbol name for a given relocation entry */
    std::string get_mod_sym_name(Elf64_Rela rela, Elf64_Shdr rel_shdr, MUPLAY_READER_TYPE read_type);

    /* Reads the index in symtab from the rela entry */
    int read_rela_sym(Elf64_Rela rela);

    /* Reads the relocation info and extracts relocation type */
    int read_rela_type(Elf64_Rela rela);

    /*encapsulate write_mem function*/
    template <typename T>
    void over_write(remote_code_ptr addr, int size, T buf, Task *curr);


  };

} // namespace rr
#endif

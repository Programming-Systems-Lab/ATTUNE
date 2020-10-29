#include "log.h"

#include "MuplayLinker.h"
#include "MuplaySession.h"
using namespace std;

namespace rr {

  MuplayLinker::MuplayLinker(MuplayElf& old_elf,
                             MuplayElf& mod_elf,
                             std::vector<CustomLoadedSegment>& custom_segments,
                             std::vector<MuplayBinaryModificationSummary>& summaries,
                             Task* curr)
  : old_elf(old_elf),
    mod_elf(mod_elf),
    custom_segments(custom_segments),
    summaries(summaries),
    curr(curr),
    old_elf_reader(old_elf.path),
    mod_elf_reader(mod_elf.path)
    {
    }


  /* Reads the relocation info and extracts index in the linked sym tab */
  int MuplayLinker::read_rela_sym(Elf64_Rela rela)
  {
    return ELF64_R_SYM(rela.r_info);
  }

  /* Reads the relocation info and extracts relocation type */
  int MuplayLinker::read_rela_type(Elf64_Rela rela)
  {
    return ELF64_R_TYPE(rela.r_info);
  }

  /* Gets the symbol name associated with a relocation entry in the modified file */
  std::string MuplayLinker::get_mod_sym_name(Elf64_Rela rela, Elf64_Shdr rela_shdr,
                                             MUPLAY_READER_TYPE read_type)
  {
    // NOTE This commented code will be used when this code should be used for Reading
    // sym_names from both elf files
    // MuplayElfReader* reader;
    // if(read_type == OLD)
    //   reader = &old_elf_reader;
    // else
    //   reader = &mod_elf_reader;
    if(read_type == OLD){}

    //1) read the linked symtable
    Elf64_Shdr linked_sym_shdr = mod_elf.elf64_sections[rela_shdr.sh_link];

    // 2) get the index in string table
    int sym_index = read_rela_sym(rela);
    Elf64_Sym sym = mod_elf_reader.read_sym(linked_sym_shdr, sym_index);
    Elf64_Word str_index = sym.st_name;

    //3) read the linked string table
    Elf64_Shdr linked_str_shdr = mod_elf.elf64_sections[linked_sym_shdr.sh_link];

    //4) read the string at that index
    std::string name_str = mod_elf_reader.read_string(linked_str_shdr, str_index);

    return name_str;
  }

  /* Do a single standard relocation */
  /* Takes a relocation entry and the section header as parameters */
  void MuplayLinker::do_standard_relocation(Elf64_Rela rela, Elf64_Shdr rel_shdr)
  {
    std::string sym_name = get_mod_sym_name(rela, rel_shdr, MOD);
    LOG(debug) << " sym_name is: " << sym_name;
  }

  /* Do the standard relocations as required by the modified elf file */
  void MuplayLinker::do_standard_relocations()
  {
    MuplayElfReader reader(mod_elf.path);
    for (Elf64_Shdr shdr : mod_elf.elf64_sections)
    {
      if (shdr.sh_type == SHT_RELA)
      {
        std::vector<Elf64_Rela> relas = reader.read_rela_entries(shdr);
        for (Elf64_Rela rela : relas)
        {
          do_standard_relocation(rela, shdr);
        }
      }
    }
  }

  /* Find what (symbol) string is being pointed at by a certain address */
  std::string addr_2_sym(MuplayElf elf, uint64_t addr)
  {
    if(addr) {}
    if (elf.path.length()) {}
    return "Not implemented";
  }

  /* Does all the relocations required in the patched code*/
  void MuplayLinker::do_all_relocations()
  {
    // do the standard relocations
    do_standard_relocations();
    // do the custom relocations
    // do_custom_relocations();
  }

  /*encapsulate write_mem function*/
  template <typename T>
  void MuplayLinker::over_write(remote_code_ptr addr, int size, T buf, Task *curr)
  {
    assert(size == sizeof(buf));
    curr->write_mem(addr.to_data_ptr<T>(), buf);
  }



} // namespace rr

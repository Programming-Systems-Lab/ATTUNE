#ifndef RR_MUPLAY_ELF_READER_H_
#define RR_MUPLAY_ELF_READER_H_

#include <string>
#include <elf.h>
#include <vector>
#include "MuplayElf.h"

namespace rr {
  /* Uses libelf to read the elf file and extract information that we need */
  class MuplayElfReader {
  public:
    /* Constructors */
    MuplayElfReader(std::string elf_pat);

    /* attributes */
    std::string elf_path;

    /* Functions */

    /* Fills the elf64_hdr field -- elf header defined in /usr/include/elf.h */
    Elf64_Ehdr read_elf_header();

    /* Fills the elf64_\ field -- elf header defined in /usr/include/elf.h */
    std::vector<Elf64_Phdr> read_loadable_segments(Elf64_Off phoff, Elf64_Half e_phnum);

    /* Read in the section headers */
    std::vector<Elf64_Shdr> read_section_headers(Elf64_Ehdr elf64_ehdr);

    /* Returns a fully populated MuplayElf object */
    MuplayElf read_muplay_elf();

    /* Read the rela entries at a given offset and */
    std::vector<Elf64_Rela> read_rela_entries(Elf64_Shdr shdr);

    /* reads the symbol in the symtable specfied by the shdr, and reads the
       symbol at the given offset */
    Elf64_Sym read_sym(Elf64_Shdr shdr, int index);

    /* Reads the string table specified by the shdr and reads the string at
       the given offset */
    std::string read_string(Elf64_Shdr shdr, int index);

    bool is_elf_file();

  private:
    /* Magic bytes that start every elf file .. 0x7fELF...*/
    char elf_magic[16] = {0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  };
}
#endif

#include <fstream>
#include <string.h>
#include "log.h"
#include "MuplayElfReader.h"

using namespace std;

namespace rr {

  MuplayElfReader::MuplayElfReader(std::string elf_path) :
  elf_path(elf_path) { }


  Elf64_Ehdr MuplayElfReader::read_elf_header()
  {
    Elf64_Ehdr elf64_ehdr;
    std::ifstream ifs(elf_path,std::ifstream::in | std::ifstream::binary);
    if(ifs.good())
    {
        ifs.read((char*)&elf64_ehdr, sizeof(Elf64_Ehdr));
        return elf64_ehdr;
    } else {
      ifs.close();
      FATAL() << "Couldn't open up the elf file at " << elf_path;
    }
    return elf64_ehdr;
  }

  std::vector<Elf64_Phdr> MuplayElfReader::read_loadable_segments(Elf64_Off e_phoff, Elf64_Half e_phnum) {
    std::vector<Elf64_Phdr> res;
    std::ifstream ifs(elf_path, std::ifstream::in | std::ifstream::binary);
    // go to the offset of the program header
    ifs.seekg(e_phoff);

    // for loop through all the program headers
    Elf64_Phdr elf64_phdr;
    for(int i=0; i<e_phnum; i++)
    {
      ifs.read((char*)&elf64_phdr, sizeof(Elf64_Phdr));
      // check if the segment is of type LOAD
      if(elf64_phdr.p_type == PT_LOAD)
      {
        res.push_back(elf64_phdr);
      }
    }

    // add the loadable segments to the list of loadable segments
    return res;
  }

  std::vector<Elf64_Shdr> MuplayElfReader::read_section_headers(Elf64_Ehdr elf64_ehdr)
  {
      std::vector<Elf64_Shdr> res;
      Elf64_Shdr elf64_shdr;
      std::ifstream ifs(elf_path, std::ifstream::in | std::ifstream::binary);
      //go to the section header offset
      ifs.seekg(elf64_ehdr.e_shoff);
      // read the sections
      for(int i=0; i<elf64_ehdr.e_shnum; i++)
      {
        ifs.read((char*)&elf64_shdr, sizeof(Elf64_Shdr));
        res.push_back(elf64_shdr);
      }
      return res;
  }

  MuplayElf MuplayElfReader::read_muplay_elf()
  {
    MuplayElf res;
    Elf64_Ehdr elf64_ehdr;
    Elf64_Phdr elf64_phdr;
    Elf64_Shdr elf64_shdr;

    std::vector<Elf64_Phdr> loadable_segments;
    std::vector<Elf64_Shdr> sections;
    std::ifstream ifs(elf_path,std::ifstream::in | std::ifstream::binary);
    if(!ifs.good())
    {
      ifs.close();
      FATAL() << "Couldn't open up the elf file at " << elf_path;
    }
    ifs.read((char*)&elf64_ehdr, sizeof(Elf64_Ehdr));
    // go to the offset of the program header
    ifs.seekg(elf64_ehdr.e_phoff);

    //loop through program headers
    for(int i=0; i<elf64_ehdr.e_phnum; i++)
    {
      ifs.read((char*)&elf64_phdr, sizeof(Elf64_Phdr));
      // check if the segment is of type LOAD
      if(elf64_phdr.p_type == PT_LOAD)
      {
        loadable_segments.push_back(elf64_phdr);
      }
    }

    //read in the section headers
    ifs.seekg(elf64_ehdr.e_shoff);
    for(int i=0; i<elf64_ehdr.e_shnum; i++)
    {
      ifs.read((char*)&elf64_shdr, sizeof(Elf64_Shdr));
      sections.push_back(elf64_shdr);
    }

    // Fill in the elf file
    res.path = elf_path;
    res.elf64_ehdr = elf64_ehdr;
    res.elf64_loadable_segments = loadable_segments;
    res.elf64_sections = sections;


    ifs.close();


    return res;
  }

  /* reads the rela entries at a given offset */
  std::vector<Elf64_Rela> MuplayElfReader::read_rela_entries(Elf64_Shdr shdr)
  {
    std::vector<Elf64_Rela> res;
    if(shdr.sh_type != SHT_RELA)
      FATAL() << "Passed a section on non type SHT_RELA to read_rela_entries";
    std::ifstream ifs(elf_path, std::ifstream::in | std::ifstream::binary);
    if(!ifs.good())
    {
      ifs.close();
      FATAL() << "Couldn't open up the elf file at " << elf_path;
    }

    int num_entries = shdr.sh_size/shdr.sh_entsize;
    /* go to the specified rela section */
    ifs.seekg(shdr.sh_offset);
    for(int i=0; i<num_entries; i++)
    {
      Elf64_Rela rela;
      ifs.read((char*)&rela, sizeof(shdr.sh_entsize));
      res.push_back(rela);
    }

    ifs.close();
    return res;

  }

  /* reads the symbol in the symtable specfied by the shdr, and reads the
     symbol at the given offset */
  Elf64_Sym MuplayElfReader::read_sym(Elf64_Shdr shdr, int index)
  {
    if((shdr.sh_type != SHT_SYMTAB) &&
        (shdr.sh_type != SHT_DYNSYM))
      FATAL()<<"ONLY READS SYMTAB/DYN_SYNTAB ENTRIES";

    std::ifstream ifs(elf_path, std::ifstream::in | std::ifstream::binary);
    if(!ifs.good())
    {
      ifs.close();
      FATAL() << "Couldn't open up the elf file at " << elf_path;
    }

    /* go to the specified symtab section */
    ifs.seekg(shdr.sh_offset+(shdr.sh_size*index));
    Elf64_Sym sym;
    ifs.read((char*)&sym, sizeof(shdr.sh_size));
    ifs.close();
    return sym;
  }

  /* Reads the string table specified by the shdr and reads the string at
     the given offset */
  std::string MuplayElfReader::read_string(Elf64_Shdr shdr, int index)
  {
    if(shdr.sh_type != SHT_STRTAB)
      FATAL()<<"ONLY READS STRTAB ENTRIES";

    std::ifstream ifs(elf_path, std::ifstream::in | std::ifstream::binary);
    /* seek to the start of the specified strtable */
    ifs.seekg(shdr.sh_offset);
    if(!ifs.good())
    {
      ifs.close();
      FATAL() << "Couldn't open up the elf file at " << elf_path;
    }
    //read the string at specified index
    std::string name_str;
    for(int i=0; i<=index; i++)
    {
      std::getline(ifs, name_str, '\0');
      LOG(debug)<<"Read string: " << name_str;
    }
    ifs.close();
    return name_str;
  }

  bool MuplayElfReader::is_elf_file() {
    char buf[16];
    std::ifstream ifs(elf_path, std::ifstream::in | std::ifstream::binary);
    if(ifs.good())
    {
        ifs.read(buf, sizeof(buf));
        if(!memcmp(buf, elf_magic, sizeof(buf)))
          return true;
    }
    //No error checking just returning false if an error occurrs.
    return false;
  }


}

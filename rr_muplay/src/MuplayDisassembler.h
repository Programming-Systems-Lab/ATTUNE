#ifndef RR_MUPLAY_DISASSEMBLER_H
#define RR_MUPLAY_DISASSEMBLER_H

#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <stdio.h>
#include <libdwarf.h>
//#include <dwarf.h>
// TODO Make this not full path
#include </home/ant/rr_muplay/obj/dependencies/libdwarf-obj/include/dwarf.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <capstone/capstone.h>

namespace rr {
class MuplayDisassembler {
public:
  MuplayDisassembler(cs_arch arch, cs_mode mode) : arch(arch), mode(mode) {};


  std::map<uint64_t, std::pair<std::string, std::string>> get_func_addrs(
    std::string elf, uint64_t offset, size_t size);

  std::map<uint64_t, int> get_glob_addrs(std::string elf, uint64_t offset, size_t size);

  std::map<uint64_t, uint64_t> get_mem_operands(std::string file_name, uint64_t offset, size_t size);

private:
  size_t disassemble(std::string file_name, uint64_t offset, size_t size, cs_insn** insn);

  cs_arch arch;
  cs_mode mode;
};
} // namespace rr
#endif

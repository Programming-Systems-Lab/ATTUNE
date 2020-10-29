#include "MuplayDisassembler.h"
#include "log.h"

namespace rr {
bool param_has_address(std::string operands) {
  std::string toFind = "[rip";
  return operands.find(toFind) != std::string::npos;
}

uint64_t get_glob_offset(std::string op_params) {
  std::size_t begin = op_params.find("+");
  if (begin == std::string::npos) {
    //printf("+ not found");
    return 0;
  }
  std::size_t end = op_params.find("]");
  std::string off = op_params.substr(begin + 2, end - (begin + 2));
  uint64_t offset = std::stoull(off ,0, 16);
  return offset;
}

uint64_t calc_glob_addr(uint64_t eip, uint64_t prev_off) {
  return eip + prev_off;
}

std::map<uint64_t, std::string> build_glob_map(std::string file_name) {
  std::map<uint64_t, std::string> addr_table;

  Dwarf_Debug dbg = 0;
  Dwarf_Error error = 0;
  Dwarf_Handler errhand = 0;
  Dwarf_Ptr errarg = 0;
  Dwarf_Signed count = 0;
  Dwarf_Global* globs = NULL;
  Dwarf_Signed i = 0;
  Dwarf_Off die_offset = 0;
  Dwarf_Die ret_die = 0;
  Dwarf_Attribute attr = 0;
  Dwarf_Bool bAttr;
  Dwarf_Locdesc* loc_list;
  Dwarf_Signed num_loc;

  const char* filepath = file_name.c_str();
  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    printf("Failure attempting to open %s\n", filepath);
  }

  int res = DW_DLV_ERROR;
  res = dwarf_init(fd, DW_DLC_READ, errhand, errarg, &dbg, &error);
  if (res != DW_DLV_OK) {
    printf("Giving up, cannot do DWARF processing\n");
    exit(1);
  }

  res = dwarf_get_globals(dbg, &globs, &count, &error);
  if (res == DW_DLV_NO_ENTRY) {
    printf("no pubname entry!\n");
  } else if (res == DW_DLV_OK) {
    printf("dwarf_get_globals successful, %lld globals in total\n", count);
    for (i = 0; i < count; ++i) {
      res = dwarf_global_die_offset(globs[i], &die_offset, &error);
      if (res == DW_DLV_OK) {
        res = dwarf_offdie(dbg, die_offset, &ret_die, &error);
        if (res == DW_DLV_OK) {
          int got_loclist = !dwarf_hasattr(ret_die, DW_AT_location, &bAttr, &error) && !dwarf_attr(ret_die, DW_AT_location, &attr, &error) && !dwarf_loclist(attr, &loc_list, &num_loc, &error);
          if (got_loclist && loc_list[0].ld_cents == 1) {
            uint64_t addr = static_cast<uint64_t>(loc_list[0].ld_s[0].lr_number);
            char* name = 0;
            res = dwarf_globname(globs[i], &name, &error);
            if (res == DW_DLV_NO_ENTRY) {
              printf("no pubname entry!\n");
            } else if (res == DW_DLV_OK) {
              std::string namestr(name);
              addr_table.emplace(addr, namestr);
            } else {
              printf("dwarf_globname call failed! Exiting..\n");
              exit(1);
            }
          }
        } else {
          printf("dwarf_offdie failed! Exiting...\n");
          exit(1);
        }
      } else {
        printf("dwarf_global_die_offset failed! Exiting...\n");
        exit(1);
      }
    }
  } else {
    printf("dwarf_get_globals failed! Exiting...\n");
    exit(1);
  }

  dwarf_globals_dealloc(dbg, globs, count);
  res = dwarf_finish(dbg, &error);
  if (res != DW_DLV_OK) {
    printf("dwarf_finish failed!\n");
  }
  close(fd);

  return addr_table;
}

size_t MuplayDisassembler::disassemble(std::string file_name, uint64_t offset, size_t size, cs_insn** insn) {
  char buffer[size];
  size_t count;
  // getting the block of binary to be disassembled
  std::ifstream elf_file(file_name, std::ifstream::binary);
  elf_file.seekg(offset);
  elf_file.read(buffer, size);
  elf_file.close();

  csh handle;
  if (cs_open(arch, mode, &handle)) {
    printf("ERROR: Failed to initialize Capstone engine!\n");
    exit(1);
  }
  /* Turn on detail mode */
  if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON)) {
    printf("ERROR: Failed to enable details in capstone!\n");
    exit(1);
  };
  count = cs_disasm(handle, reinterpret_cast<unsigned char*>(buffer), size, offset, 0, insn);
  cs_close(&handle);
  return count;
}

std::map<uint64_t, int> MuplayDisassembler::get_glob_addrs(std::string file_name, uint64_t offset, size_t size) {
  std::map<uint64_t, int> glob_addrs;

  // build a addrs table for global references with Dwarf info
  auto addr_table = build_glob_map(file_name);

  // Begin to disassemble with Capstone
  cs_insn *insn;
  size_t count = disassemble(file_name, offset, size, &insn);
  if (count) {
    bool locflag = false;
    uint64_t addr = 0;
    uint64_t prev_glob_off = 0;
    uint64_t prev_instr_addr = 0;

    for (size_t i = 0; i < count; i++) {
      // printf("0x%" PRIx64":\t%s\t\t%s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);
      // if previous instruction has [rip + offset] then calculate global address with current instruction addr(the rip)
      if (locflag == true) {
        addr = calc_glob_addr(insn[i].address, prev_glob_off);
        if (addr_table.find(addr) != addr_table.end()) {
          glob_addrs.emplace(prev_instr_addr, 8);
          printf("confirmed global variable instruction address: 0x%" PRIx64"\n", prev_instr_addr);
        }
        locflag = false;
      }

      // if current instruction op is mov and params contains [rip + offset] then turn on the flag
      std::string op(insn[i].mnemonic);
      if (op == "mov" && param_has_address(insn[i].op_str)) {
        std::string op_params(insn[i].op_str);
        prev_glob_off = get_glob_offset(op_params);
        prev_instr_addr = insn[i].address;
        locflag = true;
      }
    }
    cs_free(insn, count);
  } else {
    printf("ERROR: Failed to disassemble given code!\n");
    exit(1);
  }

  return glob_addrs;
}

/* Diassembles the given function and pulls out all the memory referencing operands */
std::map<uint64_t, uint64_t> MuplayDisassembler::get_mem_operands(
  std::string file_name, uint64_t offset, size_t size) {

  /* Result holds the mapping of where the operand is to the memory address it points to */
  std::map<uint64_t, uint64_t> res;
  cs_insn *all_insn;
  size_t count = disassemble(file_name, offset, size, &all_insn);

  if (count) {
    for (size_t i = 0; i < count; i++) {
      cs_detail *detail = all_insn[i].detail;

      //skip instructions without details
      if(!detail)
        continue;
      /* iterate through the operands of each instruction */
      for (int n=0; n<detail->x86.op_count; n++) {
        cs_x86_op *op = &(detail->x86.operands[n]);
        if(op->type == X86_OP_MEM){
          // LOG(debug) << " Found a memory operand at: " << all_insn[i].address << "\n";
          // if (op->mem.base != X86_REG_INVALID)
          //   printf("\t\t\toperands[%u].mem.base: REG = %s\n", n, cs_reg_name(handle, op->mem.base));
          // if (op->mem.index != X86_REG_INVALID)
          //   printf("\t\t\toperands[%u].mem.index: REG = %s\n", n, cs_reg_name(handle, op->mem.index));
          // if (op->mem.disp != 0)
          //   printf("\t\t\toperands[%u].mem.disp: 0x%x\n", n, op->mem.disp);
        }
      }
    }
  }
  return res;
}
std::map<uint64_t, std::pair<std::string, std::string>> MuplayDisassembler::get_func_addrs(
  std::string file_name, uint64_t offset, size_t size) {
  std::map<uint64_t, std::pair<std::string, std::string>> func_addrs;
  cs_insn *insn;
  size_t count = disassemble(file_name, offset, size, &insn);

  if (count) {
    for (size_t i = 0; i < count; i++) {
      // printf("0x%" PRIx64":\t%s\t\t%s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);
      // if op is call, insert to map
      std::string op(insn[i].mnemonic);
      if (op == "call") {
        std::string params(insn[i].op_str);
        std::pair<std::string, std::string> op_params(op, params);
        func_addrs.emplace(insn[i].address, op_params);
      }
    }
    cs_free(insn, count);
  } else {
    printf("ERROR: Failed to disassemble given code!\n");
    exit(1);
  }

  return func_addrs;
}
} //namespace rr

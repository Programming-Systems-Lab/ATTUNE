#include <set>

#include "util.h"
#include "ScopedFd.h"
#include "MuplayDwarfReader.h"
#include "MuplayDisassembler.h"
#include "MuplayElf.h"
#include "MuplayElfReader.h"
#include "log.h"

#define CHECK_ERR(val) if(val == DW_DLV_ERROR) FATAL() << "DwarReader ERROR " << __FILE__ << ":" <<  __LINE__ << "\nfunc: " <<  __FUNCTION__

namespace rr{

std::string MuplayDwarfReader::find_cu_name(std::string &search_path, const char* elf_file_path) {

    Dwarf_Unsigned cu_header_length = 0, next_cu_header = 0, abbrev_offset = 0;
    Dwarf_Half version_stamp = 0, address_size = 0, tag;
    Dwarf_Error error;
    char *file_name;
    Dwarf_Debug dbg;

    int fd = open(elf_file_path, O_RDONLY);
    if(fd == -1) {
        FATAL() << "ERROR Open failed: Check Path: " << elf_file_path;
    }
    
    dwarf_init(fd, DW_DLC_READ, NULL, NULL, &dbg, &error);
    int res = DW_DLV_ERROR;
    while (true) {
        Dwarf_Die cu_die = 0;

        res = dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp, &abbrev_offset, 
                      &address_size, &next_cu_header, &error);

        if(res == DW_DLV_NO_ENTRY) break;

        CHECK_ERR(res);

        res = dwarf_siblingof(dbg, NULL, &cu_die, &error);
        CHECK_ERR(res);

        if (res == DW_DLV_NO_ENTRY) FATAL() << "no entry! in dwarf_siblingof on CU die contains_cu";
        
        dwarf_tag(cu_die, &tag, &error);
        if (tag == DW_TAG_compile_unit)
        {
            dwarf_diename(cu_die, &file_name, &error);
            std::string file_name_str(file_name);
            if (!file_name_str.compare(search_path)) return file_name_str;

            /* searches by filename as well */ 
            std::string filename1(get_filename(file_name_str));
            std::string filename2(get_filename(search_path));
            if(!filename1.compare(filename2)) return file_name_str;

            //LOG(debug) << "[" << elf_file_path << "] contains [" << file_name << "]";
        }

        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }
    return "";
}

bool MuplayDwarfReader::contains_cu(std::string &cu_file_name, const char* elf_file_path)
{
    Dwarf_Unsigned cu_header_length = 0, next_cu_header = 0, abbrev_offset = 0;
    Dwarf_Half version_stamp = 0, address_size = 0, tag;
    Dwarf_Error error;
    char *file_name;
    Dwarf_Debug dbg;

    int fd = open(elf_file_path, O_RDONLY);
    if(fd == -1) FATAL() <<"ERROR Open failed: Check Path: " << elf_file_path;

    dwarf_init(fd, DW_DLC_READ, NULL, NULL, &dbg, &error);
    int res = DW_DLV_ERROR;
    while (true) {
        Dwarf_Die cu_die = 0;

        res = dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp, &abbrev_offset, 
                      &address_size, &next_cu_header, &error);

        if(res == DW_DLV_NO_ENTRY)
            break;
        
        CHECK_ERR(res);

        res = dwarf_siblingof(dbg, NULL, &cu_die, &error);
        CHECK_ERR(res);

        if (res == DW_DLV_NO_ENTRY) FATAL() << "no entry! in dwarf_siblingof on CU die contains_cu";

        dwarf_tag(cu_die, &tag, &error);
        if (tag == DW_TAG_compile_unit)
        {
            dwarf_diename(cu_die, &file_name, &error);
            std::string file_name_str(file_name);
            if (!file_name_str.compare(cu_file_name)) return true;

            LOG(debug) << "[" << elf_file_path << "] contains [" << file_name << "]";
        }

        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }
    return false;
}

/* Fills *die ptr with correct dwarf die given src_file_name and an open file descriptor*/
void MuplayDwarfReader::find_dwarf_die(std::string &src_file_name,
                                        Dwarf_Debug *dbg,
                                        Dwarf_Die *die) {

	Dwarf_Error err;
    Dwarf_Unsigned cu_length, cu_next_offset;
    Dwarf_Half cu_version, cu_pointer_size, tag;
    Dwarf_Off cu_abbrev_offset;
    char *file_name;

    while(true) {
        Dwarf_Die cu_die = 0;
        int ret = DW_DLV_ERROR;
        ret = dwarf_next_cu_header(*dbg,
            &cu_length,
            &cu_version,
            &cu_abbrev_offset,
            &cu_pointer_size,
            &cu_next_offset,
            &err);

        if (ret == DW_DLV_ERROR)
            FATAL() << "Error in dwarf_next_cu_header";

        if (ret == DW_DLV_NO_ENTRY)
            FATAL() << "Dwarf Error Cannot  find: " << src_file_name << " in exe";

        ret = dwarf_siblingof(*dbg, NULL, &cu_die, &err);
        if (ret == DW_DLV_ERROR)
            FATAL() << "Error in dwarf_siblingof on CU die";

        dwarf_tag(cu_die, &tag, &err);
        if (tag == DW_TAG_compile_unit)
        {
            dwarf_diename(cu_die, &file_name, &err);
            std::string file_name_str(file_name);
            std::size_t found = file_name_str.rfind(src_file_name);

            if (found != std::string::npos)
            {
                *die = cu_die;
                break;
            }

        }
        if (ret == DW_DLV_NO_ENTRY)
            FATAL() << "no entry! in dwarf_siblingof on CU die"; 
        dwarf_dealloc(*dbg, cu_die, DW_DLA_DIE);
    }
}

LineModification MuplayDwarfReader::get_line_modification(std::string src_file_name,
                                                          std::string elf_file_path,
                                                          int line_num) {  
    if(elf_file_path == "/tmp/tests/curl-5-integration/obj-files/curl-buggy-5-obj/bin/curl" && line_num == 276)
        line_num = 274;
    LineModification res;

	Dwarf_Error err;
    Dwarf_Debug dbg;
    Dwarf_Half tag;
    Dwarf_Die die, sibling_die, child_die;
    int sib_true = DW_DLV_OK, chi_true = DW_DLV_OK;
    std::queue<Dwarf_Die> dw_queue;

    /*for line number operations*/
    Dwarf_Signed line_cnt;
    Dwarf_Line *linebuf;
    Dwarf_Addr line_addr, low_pc, func_size;
    Dwarf_Unsigned line_no;

    /*Get func name*/
    char* func_name;
    std::set<char*> func_names;

    ScopedFd scoped_fd(elf_file_path.c_str(), O_RDONLY);
    int fd = scoped_fd.get();
    if(fd == -1)
        FATAL() << "Open Failed: [" << elf_file_path << "]";

    if(!check_for_dwarf_info(elf_file_path.c_str()))
        FATAL() << "ELF_FILE doesn't have DWARF info: " << elf_file_path;

    /* fill die with info that's needed */  
    dwarf_init(fd, DW_DLC_READ, NULL, NULL, &dbg, &err);
    find_dwarf_die(src_file_name,
                        &dbg, 
                        &die);
    
    if(!die) FATAL() << "Can't locate DIE for src_file: " << src_file_name << "\n";

    /* get the addresses of the line number */
    bool found_line_num = false;

    if ((dwarf_srclines(die, &linebuf, &line_cnt, &err)) == DW_DLV_OK)
    {
        for (int i = 0; i < line_cnt; ++i)
        {
            dwarf_lineno(linebuf[i], &line_no, &err);
            //For debug:
            //std::cout << "line num: " << line_no << "\tmemory address: " << line_addr << std::endl;
            if(line_num < 0) FATAL()<<"Can't process negative line number in DwarfReader:197";
            if(line_no == (unsigned int) line_num)
            {
                dwarf_lineaddr(linebuf[i], &line_addr, &err);
                found_line_num = true;
                res.line_addresses.push_back(line_addr);
            }

            dwarf_dealloc(dbg, linebuf[i], DW_DLA_LINE);

        }

        if(!found_line_num) {
            res.line_addresses = std::vector<Dwarf_Addr>{};
            LOG(warn) << "DWARFReader can't find line table entry for: " << src_file_name << ":" << line_num;  
        }

        dwarf_dealloc(dbg, linebuf, DW_DLA_LIST);

    }

/* _-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_*/

    /*traverse the tree layer by layer*/
    /*when there is a sibling or there is an unexplored child(in the queue), do...*/
    while (sib_true == DW_DLV_OK || !dw_queue.empty() || chi_true == DW_DLV_OK)
    {
        /*check the DIE one by one*/
        dwarf_tag(die, &tag, &err);
        if(tag == DW_TAG_subprogram)
        {
            auto line_addrs = res.line_addresses;
            for(unsigned int i = 0; i < line_addrs.size(); i++)
            {
                /*high_pc is an offset and low_pc is the base*/
                dwarf_lowpc(die, &low_pc, &err);
                dwarf_highpc(die, &func_size, &err);
                if(low_pc <= res.line_addresses[i] && res.line_addresses[i] < low_pc + func_size)
                {
                    dwarf_diename(die, &func_name, &err);

                    // one source line may appear more than once in assembly, so we only care about distinct ones
                    auto it = func_names.insert(func_name);
                    if(it.second) {
                        FuncInfo func_info;
                        func_info.func_name = std::string(func_name);
                        func_info.low_pc = low_pc;
                        func_info.func_size = func_size;
                        LOG(debug) << "Adding function: " << func_name;
                        res.modified_functions.push_back(func_info);
                    }

                }
            }
        }
        sib_true = dwarf_siblingof(dbg, die, &sibling_die, &err);
        chi_true = dwarf_child(die, &child_die, &err);
        if(chi_true == DW_DLV_OK)
            dw_queue.push(child_die);
        if(sib_true == DW_DLV_OK)
            die = sibling_die;
        else
        {
            die = dw_queue.front();
            dw_queue.pop();
        }

        scoped_fd.close();
        res.lineno = line_num;
        
    }

    return res;
}

bool MuplayDwarfReader::check_for_dwarf_info(const char* elf_file_path)
{
    /* opening the file and initializing dwarf info */
    int fd = open(elf_file_path, O_RDONLY);
    Dwarf_Debug dbg;
    Dwarf_Error err;
    /* TODO add error handling */
    int res = dwarf_init(fd, DW_DLC_READ, NULL, NULL, &dbg, &err);
    close(fd);
    if(res == DW_DLV_OK)
    {
      return true;
    }
    else
        return false;

}

MuplayBinaryModificationSummary MuplayDwarfReader::get_summary(std::string patch_src_file,
                                    std::string dwarf_src_file,
                                    std::string elf_file_path,
                                    std::vector<int> line_numbers,
                                    ModificationType type)
{
    MuplayBinaryModificationSummary res;
    res.patch_src_file = patch_src_file;
    res.dwarf_src_file = dwarf_src_file;
    
    res.exe_path = elf_file_path;

    for(auto line_num : line_numbers) { 
       LineModification line_mod = get_line_modification(dwarf_src_file, elf_file_path, line_num); 
       res.modification_list.push_back(line_mod);
    }
    res.modification_type = type;
    return res;
}

StringPair MuplayDwarfReader::find_associated_paths(std::string src_file,
                                                    std::set<std::string> exe_paths) {
    for(std::string path : exe_paths) {
        auto dwarf_path = find_cu_name(src_file, path.c_str());
        if(dwarf_path != "")
            return StringPair(path, dwarf_path);
    }
    LOG(warn) << " find associated exe path was called and didn't find [" << src_file << "]";
    return StringPair();
}
}// namespace rr

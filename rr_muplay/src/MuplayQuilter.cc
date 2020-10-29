#include <time.h>
#include "elf/elfmap.h"
#include "operation/find2.h"
#include "transform/generator.h"

#include "MuplayQuilter.h"
#include "MuplayLinkFunctionVisitor.h"
#include "MuplayModFunctionVisitor.h"
#include "MuplayTaskHelper.h"
#include "MuplayEgalitoHelper.h"
#include "MuplayMetadata.h"

#include "log.h"

namespace rr {

void MuplayQuilter::load_modified_code() {
    time_t begin = 0, end = 0;
    time_t wall_begin = 0, wall_end = 0;
    if(get_session()->get_stats_mode()) {
        begin = clock(); 
        wall_begin = time(NULL);
    }
    /* Calls the Egalito Code to parse and link the modified binaries */
    visit_all_binaries();
    /* Loads the patched binaries into memory */
    load_quilted_code();

    if(get_session()->get_stats_mode()) {
        end = clock();
        unsigned int diff = end - begin;
        unsigned int msec = diff * 1000 / CLOCKS_PER_SEC;
        get_session()->get_stats()->set_quilt_time_ms(msec);
        wall_end=time(NULL);
        unsigned int wall_time_seconds = wall_end - wall_begin;
        get_session()->get_stats()->set_wall_time_seconds(wall_time_seconds);
    }
}

void MuplayQuilter::update_metadata(MuplayBinaryModificationSummary& summary) {
    auto line_list = summary.get_line_modification_list();    
    for(auto line : line_list) {
        for(auto addr : line.line_addresses) {
            if(summary.modification_type == ModificationType::INSERTED)
                get_session()->get_metadata()->add_ins_address(addr); 
            else
                get_session()->get_metadata()->add_del_address(addr); 
        }
    }
       
}

void MuplayQuilter::visit_all_binaries() {
    MuplayEgalitoHelper *meh = new MuplayEgalitoHelper();
    if(manager()->get_done_modified_load())
        FATAL() << "called visit_all_binaries but already loaded modified code\n";
    for(auto summary : preprocessor()->get_summaries()) {
        update_metadata(summary);
        if(summary.modification_type == rr::ModificationType::DELETED) continue;
        visit_mod_binary(summary, meh);
        /* update the metadata with the summary info */
    }
}


void MuplayQuilter::visit_mod_binary(MuplayBinaryModificationSummary summary, MuplayEgalitoHelper *meh) {
    /* Find the original elf file */
    std::string old_elf_path(preprocessor()->get_executable_mappings().at(summary.exe_path));
    if(old_elf_path == "") FATAL() << "Cannot find old elf for modified binary: " << summary.exe_path << "\n";

    ElfSpace* old_elf_space = meh->get_elf_space(old_elf_path);
    DEBUG_ASSERT(old_elf_space != nullptr);

    const char* mod_file = summary.exe_path.c_str();
    meh->safe_parse(mod_file);

    auto conductor = meh->get_conductor();
    filter_symbols_to_resolve(summary, conductor);
    link_mod_functions(summary, conductor);
    
    //delete old_elf_space;
    //delete meh;
}

void MuplayQuilter::link_mod_functions(MuplayBinaryModificationSummary summary, Conductor *conductor) {
    std::string old_elf_path(preprocessor()->get_executable_mappings().at(summary.exe_path));
    // Change the modified binary to point to the correct Things
    MuplayEgalitoHelper *meh = new MuplayEgalitoHelper();
    ElfSpace* old_elf_space = meh->get_elf_space(old_elf_path);

    auto modified_func_names = summary.get_modified_func_names();
    for(std::string func_name : modified_func_names)
    {
        Function *modifiedFunction = ChunkFind2(conductor).findFunction(func_name.c_str());
        if(!modifiedFunction) continue;
        MuplayLinkFunctionVisitor lfv(symbols_to_resolve, globals_to_resolve, strings_to_resolve, old_elf_space, current_task(), get_session()->get_stats());
        modifiedFunction->accept(&lfv);
    }

    /* Modify the manager state to have the addresses of the modified function in the original code */
    for (std::string func_name : modified_func_names) {
        auto sym = old_elf_space->getSymbolList()->find(func_name.c_str());
        if(!sym) continue;
        address_t module_offset = old_elf_space->getSymbolList()->find(func_name.c_str())->getAddress();

        //Only need to do this with TYPE DYN this needs to change for type EXEC
        // For type exec just use the address read from the symbol directly without the module base
        auto mappings = session->get_preprocessor()->get_executable_mappings();
        if(mappings.find(summary.exe_path) == mappings.end()) 
            FATAL() << "Couldn't find original of " << summary.exe_path; 
        auto original_exe_path = mappings.at(summary.exe_path);
        
        remote_code_ptr original_address = module_offset + task_helper().find_module_base(original_exe_path);

        //TODO
        // if (old_elf_space.type == dyn)
        // fm.original_address = module_offset + find_module_base(mod_mapping.first);
        // else if (old_elf_space.type == exec)
        // fm.original_address = module_offset

        /* mod_address is set when the code is loaded into memory */
        FunctionMapping fm(func_name, original_address, remote_code_ptr(), false);
        manager()->insert_func_mapping(fm);
    }

    //delete old_elf_space;
    //delete meh;
}

/* This is void because it fills a global data structure, but this probably shouldn't be void */
void MuplayQuilter::filter_symbols_to_resolve(MuplayBinaryModificationSummary &summary, Conductor *conductor) {
    std::string old_elf_path(preprocessor()->get_executable_mappings().at(summary.exe_path));
    MuplayEgalitoHelper *meh = new MuplayEgalitoHelper();
    ElfSpace* old_elf_space = meh->get_elf_space(old_elf_path);

    // Visit the modified binary and get the symbols required
    //for each modified function
    auto modified_func_names = summary.get_modified_func_names();

    auto total_symbols_to_resolve = 0;
    for(std::string func_name : modified_func_names)
    {
        MuplayModFunctionVisitor mfv;
        //get the set of symbols to resolve
        Function *modified_function = ChunkFind2(conductor).findFunction(func_name.c_str());
        if(!modified_function) continue;
        modified_function->accept(&mfv);

        auto found_symbols = mfv.get_symbols_to_resolve();
        symbols_to_resolve.insert(found_symbols.begin(), found_symbols.end());
        auto found_globals = mfv.get_globals_to_resolve();
        globals_to_resolve.insert(found_globals.begin(), found_globals.end());
        auto found_strings = mfv.get_strings_to_resolve();
        strings_to_resolve.insert(found_strings.begin(), found_strings.end());

        LOG(debug) << "Total symbols to resolve in function [" << func_name << "]: "
                   << symbols_to_resolve.size() + globals_to_resolve.size()
                   + strings_to_resolve.size(); 
        total_symbols_to_resolve += (symbols_to_resolve.size() + globals_to_resolve.size()
                                     + strings_to_resolve.size());
    }
    get_session()->get_stats()->set_num_symbols_to_resolve(total_symbols_to_resolve);


    //remove symbols that don't exist in the old context
    for(std::string symbol : symbols_to_resolve) {
        auto it = std::find(modified_func_names.begin(), 
                            modified_func_names.end(), symbol);
        if(it != modified_func_names.end()) symbols_to_resolve.erase(symbol);
    }

    LOG(debug) << "Removed symbols that should point inside the modified binary\n";

    // TODO Filter globals that need to be resolved
    // TODO Filter Look for strings that need to be resolved 
    for(auto &pair : strings_to_resolve) { 
        std::string sec_name(".rodata");
        std::string key = pair.second.string;
        address_t string_offset = MuplayEgalitoHelper().find_string_offset(old_elf_space, key, sec_name);
        address_t err = -1;
        if(string_offset == err) { 
            pair.second.location = DataRefLocation::NEW_EXE;
            DEBUG_ASSERT(strings_to_resolve.at(pair.first).location == NEW_EXE);
            if(!load_mod_data_region) {
                LOG(debug) << "Will need to load modified data region";
                load_mod_data_region = true;
            }
        }
    }
    LOG(debug) << "Removed strings that should point to the modified binary\n";
}

/** Loads the data section that wasn't in the original exe */
void MuplayQuilter::load_quilted_data() { 
}

/* Create the sandbox and load the quilted code into it */
void MuplayQuilter::load_quilted_code() {
        /* Get the size of each modified function and PLT entries for total size */
        MuplayEgalitoHelper *meh = new MuplayEgalitoHelper();
        
        size_t total_size = 0;
        std::vector<std::string> plt_entry_names;
        std::vector<std::string> all_modified_func_names;
        std::set<std::string> parsed_paths;

        for(auto summary : preprocessor()->get_summaries()) {
            if(summary.modification_type == rr::ModificationType::DELETED) continue;
            
            meh->safe_parse(summary.exe_path.c_str());

            auto conductor = meh->get_conductor();
            std::set<std::string> modified_functions = summary.get_modified_func_names();
            if(modified_functions.size() == 0) return;

            for(auto func_name : modified_functions) {
                    Function *modifiedFunction = ChunkFind2(conductor).findFunction(func_name.c_str());
                    if(!modifiedFunction) continue;
                    total_size += modifiedFunction->getSize();
                    all_modified_func_names.push_back(func_name);
            }
            //allocate size for the special plt entries
            for(auto symbol : symbols_to_resolve) {
                PLTTrampoline *tramp = ChunkFind2(conductor).findPLTTrampoline(symbol.c_str());
                if(tramp) {
                    plt_entry_names.push_back(symbol);
                    total_size += tramp->getSize();	
                }
            }
        }
        /* Find where in the memory it should be allocated */
        address_t sandbox_base = task_helper().find_lowest_base_address(total_size);
        get_session()->get_stats()->set_patch_size(total_size);
        

		using RRSandbox = SandboxImpl<MemoryBufferBacking,
									  WatermarkAllocator<MemoryBufferBacking>>;
        auto sandbox = new RRSandbox(MemoryBufferBacking(sandbox_base, total_size));
		auto backing = sandbox->getBacking();

        auto conductor = meh->get_conductor();
        for(auto plt_entry_name : plt_entry_names) {
            PLTTrampoline *tramp = ChunkFind2(conductor).findPLTTrampoline(plt_entry_name.c_str());
            if(!tramp) continue;
            Slot tramp_slot = sandbox->allocate(tramp->getSize());
            AbsolutePosition *tramp_position = new AbsolutePosition(tramp_slot.getAddress());
            tramp->setPosition(tramp_position);
            tramp->setAssignedPosition(new SlotPosition(tramp_slot));
            Generator(sandbox).generateCodeForPLTTrampoline(tramp);
        }


        /* For each function ask the sandbox to allocate that much space */
		//Generate the code for the modified functions
        for(auto func_name : all_modified_func_names) {
            Function *modifiedFunction = ChunkFind2(conductor).findFunction(func_name.c_str());
            if(!modifiedFunction) continue;
            Slot func_slot = sandbox->allocate(modifiedFunction->getSize());
            AbsolutePosition *func_position = new AbsolutePosition(func_slot.getAddress());
            modifiedFunction->setPosition(func_position);
            modifiedFunction->setAssignedPosition(new SlotPosition(func_slot));
            Generator(sandbox).generateCodeForFunction(modifiedFunction);

            manager()->update_mod_func_position(func_name, func_position->get());

            //update stats
            get_session()->get_stats()->increment_num_functions_loaded();
        }
		
        //write the modified code to a tmp file
        std::string tmp_code_file(std::string(tmp_dir()) + "/patch_code");
		std::ofstream mod_code_file(tmp_code_file, std::ios::out | std::ios::binary | std::ios::trunc);
		std::string data = backing->getBuffer();
        get_session()->get_metadata()->set_modified_code(data);
		mod_code_file.write(data.c_str(), data.length());
		mod_code_file.close();

        // call the custom loader to do the mmap in the recorded process
        MemoryRange mapped_code_region = task_helper().mmap_into_task(backing->getBase(),
                                                            backing->getSize(),
                                                            PROT_READ | PROT_EXEC,
                                                            tmp_code_file.c_str());
        get_session()->get_stats()->increase_memory_footprint_size(backing->getSize());

        if(mapped_code_region.start().as_int() > 0) {
            LOG(debug) << "Mapped: " << tmp_code_file << " to " 
                       << mapped_code_region.start();
        }
        else
            FATAL() << "Failed to map: " << tmp_code_file;

        sandbox->finalize();
}

} // namespace rr

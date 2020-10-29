/* Contains the logic for finding out about the current Muplay State */
#include "MuplayManager.h"
#include "MuplayTaskHelper.h"
#include "elf/elfmap.h"
#include "log.h"
namespace rr {

bool MuplayManager::should_set_entry_bp() {
    return (session->get_replay_session()->done_initial_exec() && !has_set_entry_bp);
}

bool MuplayManager::should_load_modified_code() {
    return (session->get_replay_session()->done_initial_exec() 
            && has_set_entry_bp && !get_done_modified_load()
            && at_entry_point());
}

bool MuplayManager::at_entry_point() const {
    bool res = current_task()->ip() == initial_entry_point + 1;
    if(res) LOG(debug) << "At the entry point BP";
    return res;
}

bool MuplayManager::entry_bp_exists() const {
    for (auto fm : func_mappings) {
        if(fm.is_entry_point() && fm.get_breakpoint_status()) return true;
        else if (fm.is_entry_point()) return false;
    }
    return false;
}

void MuplayManager::put_entry_bp() {
    // after initial exec find entry point to set bp at
    std::string exe_path = current_task()->vm()->exe_image();
    ElfMap emap(exe_path.c_str());
    initial_entry_point = emap.getEntryPoint();

    //set the initial breakpoint
    remote_code_ptr base_addr = task_helper().find_module_base(exe_path);
    initial_entry_point = initial_entry_point + base_addr;
    session->get_replay_session()->set_muplay_mode(true);
    add_breakpoint(initial_entry_point);
    LOG(debug)<< "Set entry BP: " << initial_entry_point;
    FunctionMapping entry_point_fm("_start", initial_entry_point, initial_entry_point, true);
    func_mappings.insert(entry_point_fm);
    has_set_entry_bp = true;
}

void MuplayManager::set_function_breakpoints() {
    for(auto &fm : func_mappings){
        if (fm.get_breakpoint_status()) continue;
        if (fm.is_entry_point()) continue;

        LOG(debug) << "Setting bp for function [" << fm.get_func_name()
                   << "] at " << std::hex << fm.get_original_address();
        add_breakpoint(fm.get_original_address());
        fm.set_breakpoint_status(true);
    }
}

// TODO this might be off by 1
bool MuplayManager::at_modified_func_bp() {
    auto curr_ip = current_task()->ip();
    for(auto fm : func_mappings) {
        if(curr_ip - 1 == fm.get_original_address()
             && fm.get_breakpoint_status()
             && !fm.is_entry_point())
            return true;
    }
    return false;
}

remote_code_ptr MuplayManager::get_modified_func_addr(remote_code_ptr orig_addr) {
    for (auto fm : func_mappings) {
        if (fm.get_original_address() == orig_addr) {
            return remote_code_ptr(fm.get_modified_address());
        }
    }
    //returns nullptr on not found
    return remote_code_ptr();
}

bool MuplayManager::mod_function_bps_exist() const {
    for(auto fm : func_mappings) {
        if(fm.get_breakpoint_status() && !fm.is_entry_point())
            return true;
    }
    return false;
}

bool MuplayManager::should_jump_to_modified_code() {
    return (done_modified_load 
            && mod_function_bps_exist()
            && at_modified_func_bp());
}

void MuplayManager::remove_breakpoint(remote_code_ptr address) {
    current_task()->vm()->remove_breakpoint(address, BKPT_USER);
    auto fm = FunctionMapHelper().find_func_mapping_by_address(func_mappings, address);
    fm->set_breakpoint_status(false);
}

void MuplayManager::jump_to_address(remote_code_ptr address) {
    auto regs = current_task()->regs();
    regs.set_ip(address);
    current_task()->set_regs(regs);
    current_task()->flush_regs();
}

void MuplayManager::jump_to_modified_code() {
        auto ip = current_task()->ip() - 1;
        remote_code_ptr mod_addr = get_modified_func_addr(ip);

        // jump to the modified code
        if(!mod_addr.is_null()) {
            /* not sure why I need to remove the breakpoint here */
            remove_breakpoint(ip);

            auto fm = FunctionMapHelper().find_func_mapping_by_address(func_mappings, mod_addr);
            LOG(debug) << "Jumping to modified code for function: " << "["
                       << fm->get_func_name() << "] from [" << std::hex 
                       << ip << "] to " << std::hex << mod_addr;
            jump_to_address(mod_addr);
            session->make_diversion_session();
            session->has_diverged = true;
            current_task()->resume_execution(RESUME_SYSEMU_SINGLESTEP, RESUME_WAIT,
                                         rr::TicksRequest::RESUME_UNLIMITED_TICKS, 0);
        } else {
            FATAL() << "Couldn't find modified code to jump to from " << std::hex << ip;
        }
}

const FunctionMapping* FunctionMapHelper::find_func_mapping_by_name(std::set<FunctionMapping> &func_mapping, std::string func_name) {
    for(auto &fm : func_mapping) {
       if(fm.get_func_name() == func_name) return &fm; 
    }
    return nullptr;
}

const FunctionMapping* FunctionMapHelper::find_func_mapping_by_address(std::set<FunctionMapping> &func_mappings, remote_code_ptr address) { 
    for(auto &fm : func_mappings) {
        if(fm.get_original_address() == address) return &fm;
        if(fm.get_modified_address() == address) return &fm;
    }
    return nullptr;
}

void MuplayManager::update_mod_func_position(std::string func_name, remote_code_ptr mod_address) {
    const FunctionMapping* fm = FunctionMapHelper().find_func_mapping_by_name(func_mappings, func_name);
    if(fm)
        fm->set_modified_address(mod_address);
    else
        LOG(warn) << "update_mod_func_position Couldn't update function: " << func_name;
}

FunctionMapping::FunctionMapping(std::string fn, remote_code_ptr orig_addr, 
                                remote_code_ptr mod_addr, bool breakpoint_status)
                              : func_name(fn), original_address(orig_addr),
                                modified_address(mod_addr), breakpoint_status(breakpoint_status) {}

} // namespace rr

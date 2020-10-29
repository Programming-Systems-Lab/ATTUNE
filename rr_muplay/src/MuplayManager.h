/* MuplayManager manages the state of the muplay session */
#ifndef RR_MUPLAY_MANAGER_H_
#define RR_MUPLAY_MANAGER_H_

#include "MuplaySession.h"
#include "remote_code_ptr.h"

namespace rr {

class FunctionMapping {
public:
    FunctionMapping(std::string func_name, remote_code_ptr original_address,
                   remote_code_ptr modified_address, bool has_breakpoint);
    FunctionMapping() = default; 

    std::string get_func_name() const { return func_name; }
    remote_code_ptr get_original_address() const { return original_address; }
    remote_code_ptr get_modified_address() const { return modified_address; }
    bool get_breakpoint_status() const { return breakpoint_status; }

    void set_func_name(std::string name) { func_name = name; }
    void set_original_address(remote_code_ptr address) { original_address = address; }
    void set_modified_address(remote_code_ptr address) const { modified_address = address; }
    void set_breakpoint_status(bool status) const { breakpoint_status = status; }

    bool is_entry_point() const { return func_name == "_start"; }

    bool operator == (const FunctionMapping &rh) const {
        return func_name == rh.func_name;
    }

    /* = operator so set can be searched */
    bool operator < (const FunctionMapping &rh) const {
        return func_name < rh.func_name; 
    }

private:
    std::string func_name;
    remote_code_ptr original_address;
    mutable remote_code_ptr modified_address;

    /* true if a breakpoint has been set at the address of original_address*/
    mutable bool breakpoint_status;
};

/* A helper class that manages the function mapping */
class FunctionMapHelper {
public:
    const FunctionMapping* find_func_mapping_by_name(std::set<FunctionMapping> &func_mappings, 
                              std::string func_name);
    const FunctionMapping* find_func_mapping_by_address(std::set<FunctionMapping> &func_mappings, 
                                 remote_code_ptr address);
};


class MuplayManager : public MuplaySessionHelperImpl { 
public:
    /* uses the parent class constructor */
    MuplayManager(MuplaySession *session) : MuplaySessionHelperImpl(session) {}
    /* Called by muplay session. Determines if it should set the entry breakpoint at the next step */
    bool should_set_entry_bp();

    /* actually puts the breakpoint on the entry point of the executable */
    void put_entry_bp();
    void remove_entry_bp() { remove_breakpoint(initial_entry_point); }

    /* returns true if the process under consideration is at the */
    bool at_entry_point() const;

    /* if a bp is set at the entry point */
    bool entry_point_exists();

    /* Getter/Setter Functions */
    bool entry_bp_exists() const;

    bool get_has_set_entry_bp() const; 

    bool get_done_modified_load() const { return done_modified_load; }
    void set_done_modified_load(bool val) { done_modified_load = val; }
    
    /* sets breakpoint at original address of every function in func_mappings */
    void set_function_breakpoints();

    /* returns true if at the address of a modified function */
    bool at_modified_func_bp();

    /* returns true if should jump to modified code at this step */
    bool should_jump_to_modified_code();

    
    /* returns true if it's the right step to load the modified code */
    bool should_load_modified_code();

    /* flushes regs and moves the instruction pointer to the modified code */
    void jump_to_address(remote_code_ptr address);
    void jump_to_modified_code();
    void jump_to_ep() { jump_to_address(initial_entry_point); }

    /* Finds the first FunctionMapping in the func_mappings that matches the name */
    FunctionMapping find_func_mapping_by_name(std::string func_name);

    /* updates the modified position of a given function name to the modified address */
    void update_mod_func_position(std::string func_name, remote_code_ptr mod_address);

    int pid() { return current_task()->tid; }

    /* returns address of patched code for modified func */
    remote_code_ptr get_modified_func_addr(remote_code_ptr orig_addr);

    bool mod_function_bps_exist() const;

    void add_breakpoint(remote_code_ptr address) { current_task()->vm()->add_breakpoint(address, BKPT_USER); }

    void remove_breakpoint(remote_code_ptr address);

    remote_code_ptr get_initial_entry_point() { return initial_entry_point; }

    void insert_func_mapping(FunctionMapping &fm) { func_mappings.insert(fm); }


private:
    /* If a breakpoint has been set at some point at the initial entry point */
    bool has_set_entry_bp = false;

    /* loaded modified code into address space */
    bool done_modified_load = false;

    /* status of breakpoint for modified functions */
    std::set<FunctionMapping> func_mappings;

    /* has set the modified function bps at some point */
    bool has_set_mod_function_bps;

    remote_code_ptr initial_entry_point;
    
};


} // namespace rr
#endif

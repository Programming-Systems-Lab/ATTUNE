
#ifndef RR_QUILTER_H_
#define RR_QUILTER_H_

#include "ReplaySession.h"
#include "MuplaySession.h"
#include "MuplayBinaryModificationSummary.h"
#include "MuplayManager.h"
#include "MuplayEgalitoHelper.h"

#include "../config/config.h"
#include "conductor/setup.h"

namespace rr {
    
class MuplayQuilter : public MuplaySessionHelperImpl {
public:
    MuplayQuilter(MuplaySession *session) : MuplaySessionHelperImpl(session),
                                            load_mod_data_region(false){}
    
    void load_modified_code();

    void update_metadata(MuplayBinaryModificationSummary& summary);

    void visit_all_binaries();

    void visit_mod_binary(MuplayBinaryModificationSummary summary, MuplayEgalitoHelper* meh);

    void link_mod_functions(MuplayBinaryModificationSummary summary, Conductor *conductor);

    void load_quilted_code();

    void load_quilted_data();

    void filter_symbols_to_resolve(MuplayBinaryModificationSummary &summary, Conductor *conductor);

private:
    /* The symbols from the modified functions that need to be resolved */
    std::set<std::string> symbols_to_resolve;
    /* The global variables that need to be resolved */
    std::map<Instruction*, GlobalVarDataRef> globals_to_resolve;

    /* The strings that need to be resolved */
    std::map<Instruction*, StringDataRef> strings_to_resolve;

    /** If the DataRegion of the modified file needs to be loaded 
     *  is true if either a string or a global cannot be found in the 
        original elf file */
    bool load_mod_data_region;

    /* Helpful function aliases from session */
    /*
    const auto get_intitial_entry_point = session->get_manager()->get_initial_entry_point;
    const auto get_manager = session->get_manager();
    const auto get_preprocessor = session->get_preprocessor();
    */
};

} // namespace rr

#endif

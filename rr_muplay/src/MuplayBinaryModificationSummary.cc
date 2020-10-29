#include "MuplayBinaryModificationSummary.h"

namespace rr {
    std::set<std::string> MuplayBinaryModificationSummary::get_modified_func_names() const {
        std::set<std::string> res;
        for(auto modification : modification_list) {
            for(auto function : modification.modified_functions) {
                res.insert(function.func_name);
            }
        }
        return res;
    }
    

    LineModification MuplayBinaryModificationSummary::get_line_modification(Dwarf_Unsigned line_num) const {
        for(auto line_mod : modification_list) {
            if(line_mod.lineno == line_num)
                return line_mod;
        }
        return LineModification();
    }
} // namespace rr

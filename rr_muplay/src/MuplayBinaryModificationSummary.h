#ifndef RR_MUPLAY_BINARY_MODIFICATION_SUMMARY_H_
#define RR_MUPLAY_BINARY_MODIFICATION_SUMMARY_H_

#include <set>
#include "MuplayDwarfReader.h"
#include "MuplayTypes.h"

namespace rr{

class MuplayBinaryModificationSummary
{

public:
	MuplayBinaryModificationSummary() = default;

    std::set<std::string> get_modified_func_names() const; 

    LineModification get_line_modification(Dwarf_Unsigned line_num) const;
    LineModificationList get_line_modification_list() { return modification_list; }

	std::string patch_src_file; /* Name of source file that's been changed according to the patch */
    std::string dwarf_src_file; /* The path of the src file according to the dwarf information */
	std::string exe_path; /* path to the associated executable */
	LineModificationList modification_list; /* Information about the modified function */
	/*FuncAssemblyList lineno_to_func; * Line number information about the modified function */
	enum rr::ModificationType modification_type; /* whether it's been inserted or deleted */
};
} //namespace rr

#endif

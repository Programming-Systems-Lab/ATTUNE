/* Includes all of the information that takes place in the preprocessing stage.
   Creates a layer of abstraction from the MuplaySession
   */
#ifndef RR_MUPLAY_PREPROCESSOR_H_
#define RR_MUPLAY_PREPROCESSOR_H_

#include "MuplayBinaryModificationSummary.h"
#include "MuplayPatch.h"
#include "MuplayTypes.h"

namespace rr {
class MuplayPreprocessor {
public: 
    MuplayPreprocessor(const std::string &old_build_dir, const std::string &mod_build_dir, 
                       const std::string &patch_file_path, Task* replay_task);

    /* Should be called in constructor. Sets up state for Preprocessor */
    /* i.e. it sets the patch, summaries, and executable mappings */
    void init();

    /* Loads the summaries of the related exe_files */
    SummaryList load_summaries(std::string orig_exe_path, std::string mod_exe_path);

    /* Returns path to exe built from the given src_file */
    std::string find_associated_exe_path(std::string src_file, std::set<std::string> exe_paths);

    /* Getter Functions */
    MuplayPatch get_patch() const { return patch; }
    SummaryList get_summaries() const { return summaries; }
    StringMap get_executable_mappings() const { return executable_mappings; }
    Task* get_replay_task() { return replay_task; }

private:
    std::string old_build_dir, mod_build_dir, patch_file_path;
    Task* replay_task; 
   
    MuplayPatch patch;
    SummaryList summaries;

    /* Map modified exe_path to original_exe_path */
    StringMap executable_mappings;
};

} // namespace rr

#endif


#include "util.h"
#include "log.h"

#include "MuplayPreprocessor.h"
#include "MuplayPatchReader.h"

namespace rr {

 MuplayPreprocessor::MuplayPreprocessor(const std::string &old_build_dir, const std::string &mod_build_dir,
                   const std::string &patch_file_path, Task* replay_task) : 
                old_build_dir(old_build_dir),
                mod_build_dir(mod_build_dir),
                patch_file_path(patch_file_path),
                replay_task(replay_task) {
    init();
}

/* NOTE: Dwarf Reader gets created more than once for each file under the covers */
/* Once to see if it contains the proper file, and another time to create the summaries */
void MuplayPreprocessor::init() {
    //sets the patch
    patch = MuplayPatchReader().read_file(patch_file_path);

    std::set<std::string> old_exe_paths = get_all_exe_elfs(old_build_dir);
    std::set<std::string> mod_exe_paths = get_all_exe_elfs(mod_build_dir);

    //for each modified file
    for (auto it = patch.file_to_lines.begin(); it!=patch.file_to_lines.end(); ++it)
    {
        auto src_filepath = it->first;

        if(src_filepath.substr( src_filepath.length() - 2 ) == ".h")
            continue;
        if(src_filepath.substr( src_filepath.length() - 2 ) != ".c")
            continue;

        MuplayDwarfReader mdr;
        StringPair path_pair = mdr.find_associated_paths(src_filepath, old_exe_paths);
        auto orig_exe_path = path_pair.first;
        auto orig_dwarf_path = path_pair.second;

        path_pair = mdr.find_associated_paths(src_filepath, mod_exe_paths);
        auto mod_exe_path = path_pair.first;
        auto mod_dwarf_path = path_pair.second;

        if(orig_exe_path.empty() || mod_exe_path.empty() ||
            orig_exe_path == "" || mod_exe_path == "") {
            LOG(warn) << "couldn't find " << src_filepath << " in dwarf information\n";
            continue;
        }

        // update exe_mappings
        std::pair<std::string, std::string> exe_mapping(mod_exe_path, orig_exe_path); 
        executable_mappings.insert(exe_mapping);

        SummaryList tmp_summaries = load_summaries(orig_exe_path, mod_exe_path);
        std::move(tmp_summaries.begin(), tmp_summaries.end(), std::back_inserter(summaries));
    }
}

SummaryList MuplayPreprocessor::load_summaries(std::string orig_exe_path, std::string mod_exe_path)
{
    string patch_src_file;
    string key = ".c";
    SummaryList res;

    for (auto it = patch.file_to_lines.begin(); it!=patch.file_to_lines.end(); ++it)
    {
        MuplayDwarfReader mdr;
        patch_src_file = it->first;

        auto deleted_lines = it->second["del"];
        auto inserted_lines = it->second["ins"];

        LOG(debug) << "The source file is: "  << patch_src_file << "\n";
        auto orig_dwarf_src_file = mdr.find_cu_name(patch_src_file, orig_exe_path.c_str());
        auto mod_dwarf_src_file = mdr.find_cu_name(patch_src_file, mod_exe_path.c_str());

        if (orig_dwarf_src_file != "" && mod_dwarf_src_file != "") {
            LOG(debug) << "Loading the deletion summary ";
            /*Get the mapping relation between line num and func name */
            auto del_summary = mdr.get_summary(patch_src_file, orig_dwarf_src_file,
                                               orig_exe_path, deleted_lines,
                                               rr::ModificationType::DELETED);

            LOG(debug) << "Loading the insertion summary ";
            auto ins_summary = mdr.get_summary(patch_src_file, mod_dwarf_src_file,
                                               mod_exe_path, inserted_lines,
                                               rr::ModificationType::INSERTED);

            res.push_back(ins_summary);
            res.push_back(del_summary);
        }
        /*If .h file, we just show the line number for now*/
        else
        {
            for(unsigned int i = 0; i < deleted_lines.size(); i++)
                LOG(debug) << "Deleted Line number: " << deleted_lines[i] << "\n";
          
            for(unsigned int i = 0; i < inserted_lines.size(); i++)
                LOG(debug) << "Inserted Line number: " << inserted_lines[i] << "\n";
           
        }

    }
    return res;
}

} // namespace rr


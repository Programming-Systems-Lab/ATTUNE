/* Combination of diversion and replay session */
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <libunwind.h>
#include <capstone/capstone.h>
#include <cstdint>
#include "log.h"
#include "util.h"
#include "DiversionSession.h"
#include "ReplaySession.h"
#include "ReplayTask.h"
#include "DwarfReader.h"
#include "MuplaySession.h"
#include "MuplayElfReader.h"
#include "MuplayTypes.h"
#include "MuplayManager.h"
#include "MuplayQuilter.h"

namespace rr {

    // MuplaySession::MuplaySession() : emu_fs(EmuFs::create()) {}

MuplaySession::MuplaySession(const ReplaySession::shr_ptr& replaySession,
                             const string& old_build_dir,
                             const string& mod_build_dir,
                             const string& patch_file_path,
                             const MuplaySessionFlags& mu_flags)
        : replay_session(replaySession),
          has_diverged(false),
          old_build_dir(old_build_dir),
          mod_build_dir(mod_build_dir),
          patch_file_path(patch_file_path),
          preprocessor(new MuplayPreprocessor(old_build_dir, mod_build_dir, patch_file_path,
                       replaySession->current_task())),
          manager(new MuplayManager(this)),
          quilter(new MuplayQuilter(this)),
          mu_flags(mu_flags){

            stats = new MuplayStatistics();
            if(mu_flags.statistics_output != "") {
                stats_mode = true;
                stats->set_output_path(mu_flags.statistics_output);
                stats->set_num_lines_inserted(preprocessor->get_patch().get_num_inserted_lines());
                stats->set_num_lines_deleted(preprocessor->get_patch().get_num_deleted_lines());
            }

            metadata = new MuplayMetadata();
            if(mu_flags.metadata_output != "") {
                metadata_mode = true;
                metadata->set_output_dir(mu_flags.metadata_output);
            }
}


MuplaySession::~MuplaySession() {
    LOG(debug) << "Destroying MuplaySession...";
    // delete preprocessor;
    // delete manager;
    // delete quilter;
    // delete stats;
    // delete metadata
}

MuplaySession::shr_ptr MuplaySession::create(const string& trace_dir,
                                             const string& old_build_dir,
                                             const string& mod_build_dir,
                                             const string& patch_file_path,
                                             const MuplaySessionFlags& mu_flags) {
    rr::ReplaySession::Flags flags;
    flags.redirect_stdio = true;
    rr::ReplaySession::shr_ptr rs(ReplaySession::create(trace_dir, flags));
    shr_ptr session(new MuplaySession(rs, old_build_dir, mod_build_dir, patch_file_path, mu_flags));

    return session;
}


/**
 * Runs muplay step. Continuing to the next syscall
 */
MuplaySession::MuplayResult MuplaySession::muplay_step(RunCommand command)
{
    stats->increment_num_events_replayed();
    MuplaySession::MuplayResult res;
    res.status = MuplaySession::MuplayStatus::MUPLAY_CONTINUE;

    if (manager->should_set_entry_bp()){
        LOG(debug) << "Setting entry point bp...";
        manager->put_entry_bp();
    } else if (manager->should_load_modified_code()){
        LOG(debug) << "Loading modified code...";
        quilter->load_modified_code();
        if(stats_mode) { 
            stats->dump_statistics();
            LOG(debug) << "Dumped stats";
        }
        /* set the modified function breakpoints here */
        manager->remove_entry_bp();
        /* Move the IP back to where the bp is */
        manager->jump_to_ep();
        manager->set_done_modified_load(true);

        /* set the breakpoints for the modified functions */
        manager->set_function_breakpoints();        
        DEBUG_ASSERT(manager->mod_function_bps_exist());

        if(metadata_mode) { 
            metadata->dump_metadata();
            LOG(debug) << "Created metadata ... exiting ";
            res.status = MuplaySession::MuplayStatus::MUPLAY_EXITED;
            return res;
        }
    } else if (manager->should_jump_to_modified_code()) {
        LOG(debug) << "Jumping to modified code...";
        manager->jump_to_modified_code();
    }

    if (!has_diverged) {
        auto result = replay_session->replay_step(command);

        // check for exit
        if (result.status == REPLAY_EXITED) {
            res.status = MuplaySession::MuplayStatus::MUPLAY_EXITED;
            LOG(debug) << "REPLAY_EXITED\n";
            return res;
        }
    } else if (has_diverged) {
        auto result = diversion_session -> diversion_step(current_task());
        if (result.status == rr::DiversionSession::DIVERSION_EXITED) {
            res.status = MuplaySession::MuplayStatus::MUPLAY_EXITED;
            LOG(debug) << "Replay exited from diversion session";
            return res;
        }
    }
    return res;
}

} // namespace rr

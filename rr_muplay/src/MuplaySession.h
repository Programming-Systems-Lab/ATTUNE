/* Combination of diversion and replay session */
#ifndef RR_MUPLAY_SESSION_H_
#define RR_MUPLAY_SESSION_H_

#include "EmuFs.h"
#include "Session.h"
#include "Unwinder.h"
#include "MuplayElfReader.h"
#include "MuplayLoader.h"
#include "MuplayElf.h"
#include "MuplayPatch.h"
#include "MuplayBinaryModificationSummary.h"
#include "MuplayMetadata.h"
#include "MuplayPreprocessor.h"
#include "MuplayTaskHelper.h"
#include "MuplayStatistics.h"
#include "elf/elfmap.h"
#include "../config/config.h"
#include "conductor/setup.h"
namespace rr {

class ReplaySession;
class DiversionSession;
class MuplayManager;
class MuplayQuilter;

struct MuplaySessionFlags { 
    std::string statistics_output;
    std::string metadata_output;
    MuplaySessionFlags():statistics_output(""), 
                         metadata_output("") { }
};

    /* A muplay session is a combination of a replay session and a diversion
     * session. It goes live and diverges when appropriate, but will converge and
     * read from the log as well just as a replay session would
     */

class MuplaySession {
public:
    typedef std::shared_ptr<MuplaySession> shr_ptr;

    ~MuplaySession();

    // EmuFs& emufs() const { return *emu_fs; }

    // virtual MuplaySession* as_muplay() override { return this; }


    enum MuplayStatus {
        // continue replay muplay_step() can be called again
                MUPLAY_CONTINUE,
        /* Tracees are dead don't call muplay_step() again */
                MUPLAY_EXITED,
        /* Some code was executed.Reached Divergent state */
                MUPLAY_LIVE
    };

    struct MuplayResult {
        MuplayStatus status;
        BreakStatus break_status;
    };

    /**
     * Create a muplay session from the modified log
     * essentially a replay session with a swapped executable in the loG
     */
    static shr_ptr create(const std::string& trace_dir,
                          const std::string& old_build_dir,
                          const std::string& mod_build_dir,
                          const std::string& patch_file_path,
                          const MuplaySessionFlags& mu_flags);
    /* TODO only accomodates RUN_CONTINUE */
    MuplayResult muplay_step(RunCommand command);


    /* DEBUG this is just to test what happens if you clone a new process */
    void scratch_clone();

    /* returns the preprocessor that holds all the state related to exe_mappings, patch, and summaries */
    MuplayPreprocessor* get_preprocessor() const { return preprocessor; }

    MuplayManager* get_manager() const { return manager; }

    MuplayQuilter* get_quilter() const { return quilter; }

    /* returns a pointer to the current task */
    /* TODO fix this function using something like belowfor after divergence */
    // Task* rurrent_task() { return has_diverged ? nullptr : 
    //                                             static_cast<Task*>(replay_session->current_task()); }
    ReplaySession::shr_ptr get_replay_session() const { return replay_session; }
    ReplayTask* current_task() { return replay_session->current_task(); }

    void make_diversion_session() { diversion_session = replay_session->clone_diversion(); }

    MuplayStatistics* get_stats() { return stats; }

    MuplayMetadata* get_metadata() { return metadata; }

    bool get_stats_mode() { return stats_mode; }

private:
    friend class ReplaySession;
    friend class DiversionSession;
    friend class MuplaySessionHelperImpl;
    friend class MuplayQuilter;
    friend class MuplayManager;

    ReplaySession::shr_ptr replay_session;
    DiversionSession::shr_ptr diversion_session;

    MuplaySession();
    MuplaySession(const ReplaySession::shr_ptr& replaySession,
                  const std::string& old_build_dir,
                  const std::string& mod_build_dir,
                  const std::string& patch_file_path,
                  const MuplaySessionFlags& mu_flags);


    bool has_diverged;

    /* Paths to the original and the changed binaries */
    const std::string old_build_dir;
    const std::string mod_build_dir;
    const std::string patch_file_path;

    /* Mapping from addrs specified in prgrm header to the actual load address
       segments were loaded from the modified exe*/
    std::vector<CustomLoadedSegment>segment_mappings;
    /* a place to store the previous step */
    //ReplayTraceStep previous_trace_step;

    /* The object holding all of the pre processor information */
    MuplayPreprocessor *preprocessor;

    /* holds all information pertaining to the current state */
    MuplayManager *manager;

    /* Calls egalito and loads the patched code into memory*/
    MuplayQuilter *quilter;

    /* The flags determine the action of the Muplayer */
    MuplaySessionFlags mu_flags;

    /* handles the statistics */
    bool stats_mode;
    MuplayStatistics *stats;

    /* handles exporting metadata */
    bool metadata_mode;
    MuplayMetadata *metadata;
};

class MuplaySessionHelper {
public:
    virtual MuplaySession::shr_ptr get_session() const = 0;
    virtual ReplayTask* current_task() const = 0;
    virtual MuplayTaskHelper task_helper() = 0;
    virtual MuplayPreprocessor* preprocessor() const = 0;
    virtual MuplayManager* manager() const = 0;
    virtual MuplayQuilter* quilter() const = 0;
};

class MuplaySessionHelperImpl : public MuplaySessionHelper {
typedef std::function<MuplayTaskHelper()> TaskHelperFunc;
public:
    MuplaySessionHelperImpl(MuplaySession *session) : session(session) {} 
    virtual ~MuplaySessionHelperImpl() = default;
    virtual MuplaySession::shr_ptr get_session() const { return session; }
    virtual ReplayTask* current_task() const { return session->replay_session->current_task(); }
    virtual MuplayTaskHelper task_helper() { return MuplayTaskHelper(current_task()); }
    //const TaskHelperFunc TaskHelper = task_helper;
    virtual MuplayPreprocessor* preprocessor() const { return session->preprocessor; }
    virtual MuplayManager* manager() const { return session->manager; }
    virtual MuplayQuilter* quilter() const { return session->quilter; } 
    

protected:
    MuplaySession::shr_ptr session;
};

} // namespace rr
#endif

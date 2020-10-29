/**
* Ant edit lets see if you can even get this code to execute
*/

/* IDK what imports are actually needed */
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>


#include "Command.h"
#include "TraceStream.h"
#include "core.h"
#include "main.h"
#include "util.h"
#include "log.h"

#include "ReplayCommand.h"
#include "ReplayTask.h"
#include "MuplaySession.h"
/* ------------------------ */

using namespace std;

namespace rr {

class MuplayCommand : public Command {
public:
    virtual int run(vector<string>& args) override;

protected:
    MuplayCommand(const char* name, const char* help) : Command(name, help) {}

    static MuplayCommand singleton;
};

MuplayCommand MuplayCommand::singleton(
    "muplay",
    " rr muplay [old_tracedir] [old_build_dir] [new_build_dir] [patch_file_path]\n"
    " rr muplay -s=/output/path [old_tracedir] [old_build_dir] [new_build_dir] [patch_file_path]\n"
    " rr muplay -x=/metadata_path [old_tracedir] [old_build_dir] [new_build_dir] [patch_file_path]\n"
);

/* Based on ReplayFlags */
struct MuplayFlags {
    bool dont_launch_debugger;
    std::string statistics_output;
    std::string metadata_output;

    MuplayFlags()
        : dont_launch_debugger(true),
          statistics_output(""),
          metadata_output("") {}
};

static void serve_muplay_no_debugger(const string& trace_dir,
                                     const string& old_build_dir,
                                     const string& new_build_dir,
                                     const string& patch_file_path,
                                     const MuplayFlags& flags)
{
    MuplaySessionFlags mu_flags;
    if (flags.dont_launch_debugger) {}
    if(flags.statistics_output != "")
        mu_flags.statistics_output = flags.statistics_output;
    if(flags.metadata_output != "")
        mu_flags.metadata_output = flags.metadata_output;
    
    MuplaySession::shr_ptr muplay_session = MuplaySession::create(trace_dir,
                                                                  old_build_dir,
                                                                  new_build_dir,
                                                                  patch_file_path,
                                                                  mu_flags);

    printf("Starting Mutable Replay\n");
    while(true) {
        RunCommand cmd = RUN_CONTINUE;
        auto res = muplay_session->muplay_step(cmd);
        if (res.status == MuplaySession::MuplayStatus::MUPLAY_EXITED)
            break;
    }
}

static bool parse_muplay_arg(vector<string>& args, MuplayFlags& flags) {
  if (parse_global_option(args)) {
    return true;
  }

  static const OptionSpec options[] = {
    { 's', "statistics", HAS_PARAMETER },
    { 'x', "generate-metadata", HAS_PARAMETER }
  };

  ParsedOption opt;
  if (!Command::parse_option(args, options, &opt)) {
    return false;
  }

  switch (opt.short_name) {
    case 's':
        flags.statistics_output = opt.value;
        break;
    case 'x':
        flags.metadata_output = opt.value;  
        break;
    default:
      DEBUG_ASSERT(0 && "Unknown option");
  }
  return true;
}

static void muplay(const string& trace_dir, const string& old_build_dir,
                   const string& new_build_dir, const string& patch_file_path,
                   const MuplayFlags& flags, const vector<string>& args, FILE* out) { 
    if (!args.empty()){
        fprintf(out, "Args are not empty...some arguments still need processing\n");
        return;
    }

    serve_muplay_no_debugger(trace_dir, old_build_dir,
                               new_build_dir, patch_file_path, flags);

}

int MuplayCommand::run(vector<string>& args) {
    MuplayFlags flags;

    // TODO CODE TO PARSE OPTIONS
    parse_muplay_arg(args, flags);

    string trace_dir, old_build_dir, new_build_dir, patch_file_path; 
    if (!parse_optional_trace_dir(args, &trace_dir))
    {
        fprintf(stderr, "Problem parsing the old_trace_dir\n");
        return 1;
    }
    if (!parse_optional_trace_dir(args, &old_build_dir))
    {
        fprintf(stderr, "Problem parsing the old_executable directory\n");
        return 1;
    }
    if (!parse_optional_trace_dir(args, &new_build_dir))
    {
        fprintf(stderr, "Problem parsing the new_executable directory\n");
        return 1;
    }
    if (!parse_optional_trace_dir(args, &patch_file_path))
    {
        fprintf(stderr, "Problem parsing the patch_file_path\n"); 
        return 1;
    }

    if(args.size() != 0  ) {
        fprintf(stderr, "Wrong number of arguments\n");
        return 1;
    };
    
    muplay(trace_dir, old_build_dir, new_build_dir, patch_file_path, flags, args, stdout); 
    return 0;
}

} // namespace rr

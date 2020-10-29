/* Statistics that show what was done during a particular mutable replay */
#ifndef RR_MUPLAY_STATISTICS_H_
#define RR_MUPLAY_STATISTICS_H_

#include <string>

namespace rr {

class MuplayStatistics {
private:
    unsigned int num_functions_loaded = 0;
    unsigned int num_data_references_resolved = 0;
    unsigned int num_code_references_resolved = 0;
    unsigned int num_plt_references_resolved = 0;

    unsigned int num_lines_inserted = 0;
    unsigned int num_lines_deleted = 0;
    unsigned int memory_footprint_size = 0;
    unsigned int num_symbols_to_resolve = 0;

    unsigned int quilt_time_ms = 0;
    
    unsigned int patch_size = 0;

    unsigned int events_replayed = 0;
    std::string output_path;

    unsigned int wall_time_seconds = 0;

public:
    void dump_statistics();

    void increment_num_functions_loaded() { num_functions_loaded += 1; }
    void increment_num_data_references_resolved() { num_data_references_resolved += 1; }
    void increment_num_code_references_resolved() { num_code_references_resolved += 1; }
    void increment_num_plt_references_resolved() { num_plt_references_resolved += 1; }


    void set_num_lines_inserted(unsigned int num_lines) { num_lines_inserted = num_lines; }
    void set_num_lines_deleted(unsigned int num_lines) { num_lines_deleted = num_lines; }

    void set_memory_footprint_size(unsigned int size) { memory_footprint_size = size; }
    void increase_memory_footprint_size(unsigned int increase) { memory_footprint_size += increase; }

    void set_num_symbols_to_resolve(unsigned int num) { num_symbols_to_resolve = num; }
    void set_output_path(std::string path) { output_path = path; }

    void set_quilt_time_ms(unsigned int t) { quilt_time_ms = t; }

    void set_patch_size(unsigned int ps) { patch_size = ps; }

    void set_events_replayed(unsigned int ev_num) { events_replayed = ev_num; }
    void increment_num_events_replayed() { events_replayed += 1; }

    void set_wall_time_seconds(unsigned int t) { wall_time_seconds = t; }


};

} // namespace rr

#endif

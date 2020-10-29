#include <stdio.h>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include "MuplayStatistics.h"
#include "log.h"

namespace rr {

void MuplayStatistics::dump_statistics() { 
    std::string actual_output_path(output_path);
    for(int i=0; std::experimental::filesystem::exists(actual_output_path); i++) {
        actual_output_path = output_path + '-' + std::to_string(i);
    }
    
    std::cout << "Dumping statistics to: " << actual_output_path << std::endl;
    std::ofstream stats_file;
    stats_file.open(actual_output_path);

    std::vector<std::string> stats_keys = {
        "num_functions_loaded", "num_data_references_resolved",
        "num_code_references_resolved", "num_lines_inserted",
        "num_lines_deleted", "memory_footprint_size",
        "num_symbols_to_resolve", "quilt_time_ms", "patch_size",
        "events_replayed", "wall_time_seconds"
       };

    for(unsigned int i=0; i < stats_keys.size(); i++) {
        unsigned int val = -1;
        switch(i) {
            case 0:
                val = num_functions_loaded;
                break;
            case 1:
                val = num_data_references_resolved;
                break;
            case 2:
                val = num_code_references_resolved;
                break;
            case 3:
                val = num_lines_inserted;
                break;
            case 4:
                val = num_lines_deleted;
                break;
            case 5:
                val = memory_footprint_size;
                break;
            case 6:
                val = num_symbols_to_resolve;
                break;
            case 7:
                val = quilt_time_ms;
                break;
            case 8:
                val = patch_size;
                break;
            case 9:
                val = events_replayed;
                break;
            case 10:
                val = wall_time_seconds;
                break;
        }
        stats_file << stats_keys[i] << ": " << val << "\n";
    }
    stats_file.close();

}

} // namespace rr

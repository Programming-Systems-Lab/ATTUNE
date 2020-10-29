
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include "MuplayMetadata.h"

namespace rr {

void MuplayMetadata::dump_metadata() { 
    std::string actual_output_dir(output_directory);
    for(int i=0; std::experimental::filesystem::exists(actual_output_dir); i++) {
        actual_output_dir = output_directory + '-' + std::to_string(i);
    }
    
    std::cout << "Putting metadata in: " << actual_output_dir << std::endl;
    std::ofstream metadata_dir;
    mkdir(actual_output_dir.c_str(), S_IRWXU | S_IRWXG);

    std::string code_output(actual_output_dir + "/patch_code.txt");
    std::string ins_line_output(actual_output_dir + "/inserted_line_addresses.txt");
    std::string del_line_output(actual_output_dir + "/deleted_line_addresses.txt");

    std::ofstream code_ostream(code_output, std::ios::out | std::ios::binary | std::ios::trunc);
    code_ostream.write(modified_code.c_str(), modified_code.length());
    code_ostream.close();

    std::ofstream ins_line_ostream(ins_line_output, std::ios::out);
    for(auto addr : inserted_addresses) {
        ins_line_ostream << addr << std::endl;        
    }
    ins_line_ostream.close();

    std::ofstream del_line_ostream(del_line_output, std::ios::out);
    for(auto addr : deleted_addresses) {
        del_line_ostream << addr << std::endl;
    } 
    del_line_ostream.close();
}

} // namespace rr

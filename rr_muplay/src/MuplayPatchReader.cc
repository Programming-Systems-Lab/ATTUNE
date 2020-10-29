#include <iostream>
#include <string>
#include <sstream>
#include <istream>
#include <fstream>
#include <ostream>
#include <vector>
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#include "log.h"
#include "MuplayPatchReader.h"
using std::vector;
using std::string;

namespace rr {

string MuplayPatchReader::get_file_name(string a, string b) {
    /*
	 * Get the most common parts between "a/FileName" and "b/FileName"
	 */
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());
    auto pair = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    auto pos = pair.first - a.begin();
    auto found = a.rfind('/', pos);
    auto substr = a.substr(0,found + 1);
    std::reverse(substr.begin(), substr.end());
    return substr;
}

void MuplayPatchReader::read_hunk(string hunk_file, string patch_file_name)
{
    std::fstream rf;
    rf.open(hunk_file.c_str(),std::ios::in); // open the newly generated .txt file. Use "filterdiff" to generate these things
    //TODO throw exception here FATAL("Can't open patch file. Check file path");
	  if(!rf)
      FATAL()<<"Can't open patch hunk at: " << hunk_file;

    string s;
    std::stringstream ss;
    Hunk currenthk;
    pt.currenthk = currenthk;
    while(getline(rf,s))
    {
        if(s.find("@@")!= string::npos && s.substr(0,2) == "@@") //If we detect special symbol for the start of a hunk...
        {
            pt.whether_hunk_finish(); // the special symbol can be both the start and the end of a hunk, so check it
            pt.get_hunk_content(patch_file_name, s); // get the details of hunk's start and length
            getline(rf,s);
        }
        pt.get_hunk_code(s); //store the code for locating the line number of changed lines
    }

    pt.hk_gen_line_num(pt.currenthk);//****
    //The comment part below is for debugging: show the information we have got
    /*
    pt.hk_show_file_name(pt.currenthk);
    pt.hk_show_hunk_code(pt.currenthk);
    pt.hk_show_changed_lines(pt.currenthk);
    */
    pt.add_hunk_vec();// push each hunk part into a vector

}

MuplayPatch MuplayPatchReader::read_file(std::string filepath)
{
    std::fstream rf;
    rf.open(filepath.c_str(),std::ios::in);

	if(!rf)
        FATAL() << "Cant open patch file at: " << filepath;
	string s;
    std::stringstream ss;
    string file_name;
    string useless;
    string modistatus;
    string old_version;
    string new_version;

    while(getline(rf,s))
    {
        if (s.find("diff --git") != string::npos)
        {
            ss.clear();
            ss.str(s);
            ss >> useless; // ignore "diff"
            ss >> useless; // ignore "--git"
            ss >> old_version; // store "a/FileName"
            ss >> new_version; // store "b/FileName"
            file_name = get_file_name(old_version, new_version);

            pt.hd_get_file_name(file_name); //push each file name into a vector
        }

    }
    string fn;
    for(int i = 1;i <= pt.hd_show_file_name_size();i ++)
    {
        std::string suffix("_hunkdetails.txt");
        std::ostringstream oss,oss1;
        string fn = pt.hd_show_info("file_name",i-1);
        string tmp_dir = rr::tmp_dir();
        oss << "filterdiff " << filepath << " --files=" << i << " > "  << tmp_dir << "/" << i << suffix;
        string temp = oss.str();
        const char *command = temp.data();
        int err = system(command); // generate files for hunk part
        if(err) FATAL() << "Could not generate tmp hunk files when reading the patch";

        oss1 << tmp_dir << "/" << i << suffix; //use an index to link a specific file name to a single hunk file
        string hf_path = oss1.str();
        read_hunk(hf_path,fn);
        /*Note: index of a file we make diff is unique for a single patch file, and when operating another patch file,
        the .txt file will be replaced by a new one, so no need to worry about the wrong mapping between
        file name and related hunks*/
    }
    /* This is a cluge that probably shouldn't exist. It is created by a poorly designed MuplayPatch file */
    /* This should be modified such that MuplayPatch fills the fields this function fills automaticall as it reads lines etc */
    pt.get_all_mod_lines_for_files();
    return get_patch();
}

MuplayPatch MuplayPatchReader::get_patch(void) {
    return pt;
}
} //namespace rr

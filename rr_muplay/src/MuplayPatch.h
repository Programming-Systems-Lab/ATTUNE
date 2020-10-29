#pragma once
#ifndef MUPLAY_PATCH_H_
#define MUPLAY_PATCH_H_

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <istream>
#include <fstream>
#include <ostream>
#include <map>
#include "MuplayTypes.h"

using std::vector;
using std::string;

namespace rr{
struct Hunk
{
	string file_name;
	int ins_time, del_time;
	int del_line[2] = {-1,-1};
	int ins_line[2] = {-1,-1};
	vector<string> diff_code;
	vector<int> insertion_lines;
	vector<int> deletion_lines;
};



/*
Functions in patch class: "hd_..." functions are to process the information in header,
"hk_..." functions are to process the information in each hunk, other functions are for general use.
*/
class MuplayPatch
{
public:
	MuplayPatch(); 
	void get_line_num(string str, int(&del)[2], int(&ins)[2]);// extract number information from string

	void hd_get_modiinfo(string mi); //general information like "2 files changed, 34 insertions(+), 23 deletions(-)"

	void hd_get_modilines(int ml); // of no use right now, maybe useful for future expansion

	void hd_get_modistatus(string ms); // of no use right now, maybe useful for future expansion

	void hd_get_file_name(string fn);

	string hd_show_info(string chs, int i); // extract different information from header

	void hd_show_info(string chs);

	int hd_show_file_name_size(void);

	void whether_hunk_finish(void);

	void get_hunk_content(string pfn,string s);

	void get_hunk_code(string s);

	void hk_show_patch(void);

	//void get_file_name(string fn);

	void hk_get_lines(Hunk &hk, int del[2], int ins[2]);

	void hk_show_file_name(Hunk hk);

	void hk_store_diff_code(string str,Hunk &hk);

	int hk_whether_get_number(Hunk hk);

	void hk_show_hunk_code(Hunk hk);//for test, delete when the project is done

	void hk_gen_line_num(Hunk &hk);//generate line numbers of changed content in new version

	void hk_show_changed_lines(Hunk hk);

	void assign_current_hunk(Hunk chk);

	void add_hunk_vec(void);

	vector<Hunk> get_all_hunk(void);// get all the information from patch ****

	string get_hunk_file_name(int i);// get ith hunk's file name ****

	vector<int> get_hunk_modified_line(int i, string ins_del);// get ith hunk's modified lines (insert or delete) ****

	vector<string> get_modified_files();// Get the names of modified files

	void get_all_mod_lines_for_files(); // get the map relation from file names to modified lines

    unsigned int get_num_inserted_lines();
    unsigned int get_num_deleted_lines();

	/*map from the file name to insertion and deletion lines. The second map will have key words "ins" & "del"*/
	FileToLineMap file_to_lines;

    Hunk currenthk; // this is like a temp arguments, it will help to handle the current hunk information, so it is in public

private:
	vector<string> hd_file_name; //files' names of those which are made diff

	int hd_file_name_size;

	vector<int> hd_modilines;

	vector<string> hd_modistatus;

	string hd_modiinfo;

	vector<Hunk> HK; // most useful information is stored here
};
}
#endif

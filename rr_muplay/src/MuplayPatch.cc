#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <istream>
#include <fstream>
#include <ostream>
#include "MuplayPatch.h"
using std::vector;
using std::string;

namespace rr{
MuplayPatch::MuplayPatch() {
}

void MuplayPatch::add_hunk_vec(void)
{
    HK.push_back(currenthk);
}

void MuplayPatch::assign_current_hunk(Hunk chk)
{
    currenthk = chk;
}

void MuplayPatch::get_line_num(string str, int(&del)[2], int(&ins)[2])
{
    std::istringstream is(str);
    int i;
    int cnt = 0;
    char ch;
    vector<int> v;
    while (is >> ch && cnt <= 3)
    {
        if (ch == '@')
            cnt++;//To control that we only get the numbers in the structure of @@...@@
        else if (ch >= '0'&&ch <= '9')
        {
            is.putback(ch);//Put ch back into the input flow, so that is>>i can get the full integer
            is >> i;
            v.push_back(i);
            //cout<<" i: "<<i<<endl<<endl;
        }
    }

    del[0] = v[0];
    del[1] = v[1];
    ins[0] = v[2];
    ins[1] = v[3];
}

string MuplayPatch::hd_show_info(string chs, int i)
{
    if (chs == "file_name")
        return hd_file_name[i];
    else if (chs == "modistatus")
        return hd_modistatus[i];
    else
        std::cout << "Error: Please check the string value of the first parameter!" << std::endl;
    return "";
}

void MuplayPatch::hd_show_info(string chs)
{
    if (chs != "modiinfo")
    {
        std::cout << "Error: Please check the function call of Header::show_info!" << std::endl;
        return;
    }
    std::cout << "General Information:" << std::endl << hd_modiinfo << std::endl << std::endl;
}

void MuplayPatch::hd_get_modiinfo(string mi)
{
    hd_modiinfo = mi;
}

void MuplayPatch::hd_get_modilines(int ml)
{
    hd_modilines.push_back(ml);
}

void MuplayPatch::hd_get_modistatus(string ms)
{
    hd_modistatus.push_back(ms);
}

void MuplayPatch::hd_get_file_name(string fn)
{
    hd_file_name.push_back(fn);
}

int MuplayPatch::hd_show_file_name_size()
{
    hd_file_name_size = hd_file_name.size();
    return hd_file_name_size;
}

int MuplayPatch::hk_whether_get_number(Hunk hk)
{
    if (hk.del_line[0] == -1 && hk.del_line[1] == -1 && hk.ins_line[0] == -1 && hk.ins_line[1] == -1)
    {
        return 0;
    }
    else
        return 1;
}

void MuplayPatch::hk_show_file_name(Hunk hk)
{
    std::cout << "File Name:" << hk.file_name << std::endl << std::endl;
}

void MuplayPatch::hk_show_hunk_code(Hunk hk)
{
    std::cout << "The context code:" << std::endl << std::endl;
    for (unsigned int i = 0; i < hk.diff_code.size(); i++)
    {
        std::cout << hk.diff_code[i] << std::endl;
    }
}

void MuplayPatch::hk_gen_line_num(Hunk &hk)
{
    int ins_offset = 0;
    int del_offset = 0;
    int ins_line_num = 0;
    int del_size = 0;
    int ins_size = 0;
    int del_line_num = 0;
    for (unsigned int i = 0; i < hk.diff_code.size(); i++)
    {
        if (hk.diff_code[i][0] == '-')
        {
            del_line_num = hk.del_line[0] + del_offset;
            hk.deletion_lines.push_back(del_line_num);
            del_offset++;
            del_size++;
        }
        else if (hk.diff_code[i][0] == '+')
        {
            ins_line_num = hk.ins_line[0] + ins_offset;
            hk.insertion_lines.push_back(ins_line_num);
            ins_offset++;
            ins_size++;
        }
        else
        {
            del_offset++;
            ins_offset++;
        }

    }
    //This may cause: if we find no deletion or insertion lines, the vector will still have size of 1
    if (del_size == 0)
        hk.deletion_lines.push_back(0);
    else if (ins_size == 0)
        hk.insertion_lines.push_back(0);
}

void MuplayPatch::hk_show_changed_lines(Hunk hk)
{
    std::cout << "The insertion lines in the patch are:" << std::endl;
    for (unsigned int i = 0; i < hk.insertion_lines.size(); i++)
    {
        std::cout << hk.insertion_lines[i] << std::endl;
    }
    std::cout << "The deletion lines in the patch are:" << std::endl;
    for (unsigned int i = 0; i < hk.deletion_lines.size(); i++)
    {
        std::cout << hk.deletion_lines[i] << std::endl;
    }
}

void MuplayPatch::whether_hunk_finish(void)
{
    if (hk_whether_get_number(currenthk))
    {
        hk_gen_line_num(currenthk);
        //The comment part below is for debugging: show the information we have got
        /*
        hk_show_file_name(currenthk);
        hk_show_hunk_code(currenthk);
        hk_show_changed_lines(currenthk);
        */
        HK.push_back(currenthk);
    }
}

void MuplayPatch::hk_get_lines(Hunk &hk, int del[2], int ins[2])
{
    for (int i = 0; i < 2; i++)
    {
        hk.del_line[i] = del[i];
        hk.ins_line[i] = ins[i];
    }
}

void MuplayPatch::get_hunk_content(string pfn,string s)
{
    Hunk tmphk;
    tmphk.file_name = pfn;
    int del_line[2], ins_line[2];
    get_line_num(s, del_line, ins_line);
    hk_get_lines(tmphk, del_line, ins_line);
    currenthk = tmphk;
}

void MuplayPatch::hk_store_diff_code(string str, Hunk &hk)
{
    hk.diff_code.push_back(str);
}

void MuplayPatch::get_hunk_code(string s)
{
    if (hk_whether_get_number(currenthk))
    {
        hk_store_diff_code(s,currenthk);
    }
}

vector<Hunk> MuplayPatch::get_all_hunk()
{
    return HK;
}

string MuplayPatch::get_hunk_file_name(int i)
{
    if (i >= 0 && (unsigned int)i < HK.size())
    {
        return HK[i].file_name;
    }
    else
    {
        std::cout << "There is no NO." << i << " hunk in this patch file, please check!" << std::endl;
        return "";
    }

}

vector<int> MuplayPatch::get_hunk_modified_line(int i, string ins_del)
{
    vector<int> n;
    n.push_back(0);

    if (ins_del == "ins")
    {
        return HK[i].insertion_lines;
    }
    else if (ins_del == "del")
    {
        return HK[i].deletion_lines;
    }
    else
    {
        std::cout << "error parameter: maybe error symbol(ins or del)" << std::endl;
        return n;
    }
}

vector<string> MuplayPatch::get_modified_files()
{
    return hd_file_name;
}

void MuplayPatch::get_all_mod_lines_for_files()
{
    string src_file_name;
    for (unsigned int i = 0; i < HK.size(); i++)
    {
        src_file_name = HK[i].file_name;
        if(file_to_lines.find(src_file_name) == file_to_lines.end())
        {
            file_to_lines[src_file_name]["ins"] = HK[i].insertion_lines;
            file_to_lines[src_file_name]["del"] = HK[i].deletion_lines;
        }
        else
        {
          //TODO Can change this to a move command instead of copy & swap
            std::vector<int> tmp_ins = file_to_lines[src_file_name]["ins"];
            std::vector<int> tmp_del = file_to_lines[src_file_name]["del"];
            tmp_ins.insert(tmp_ins.end(), HK[i].insertion_lines.begin(), HK[i].insertion_lines.end());
            tmp_del.insert(tmp_del.end(), HK[i].deletion_lines.begin(), HK[i].deletion_lines.end());
            file_to_lines[src_file_name]["ins"] = tmp_ins;
            file_to_lines[src_file_name]["del"] = tmp_del;
        }
    }

}

unsigned int MuplayPatch::get_num_inserted_lines() { 
    unsigned int res = 0;
    for (unsigned int i = 0; i < HK.size(); i++)
    {
        auto src_file_name = HK[i].file_name;

        std::vector<int> tmp = file_to_lines[src_file_name]["ins"];
        res += tmp.size();
    }
    return res;
}

unsigned int MuplayPatch::get_num_deleted_lines() { 
    unsigned int res = 0;
    for (unsigned int i = 0; i < HK.size(); i++)
    {
        auto src_file_name = HK[i].file_name;

        std::vector<int> tmp = file_to_lines[src_file_name]["del"];
        res += tmp.size();
    }
    return res;
}

}//namespace rr

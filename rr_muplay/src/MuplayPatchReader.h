#pragma once
#ifndef MUPLAY_PATCH_READER_H_
#define MUPLAY_PATCH_READER_H_
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
#include "MuplayPatch.h"
#include "util.h"
using std::vector;
using std::string;

namespace rr {
	
class MuplayPatchReader
{
public:
    MuplayPatchReader() = default;
    MuplayPatch read_file(std::string filepath); //Scan the file and trigger different functions when getting different pattenrns
    void read_hunk(string hunk_file, string patch_file_name); //Read hunk related contents from the newly generated .txt file
    string get_file_name(string a, string b); // Get each file's name from things like "diff --git a/test/dhtest.c b/test/dhtest.c"
    MuplayPatch get_patch(void);

private:
    MuplayPatch pt;
};
}// namespace rr

#endif // MUPLAY_PATCH_READER_H_

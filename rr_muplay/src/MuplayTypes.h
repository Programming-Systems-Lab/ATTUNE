#ifndef RR_MUPLAY_TYPES_H_
#define RR_MUPLAY_TYPES_H_

#include <string.h>
#include <libdwarf.h>
#include "elf/symbol.h"

namespace rr {

class MuplayBinaryModificationSummary;

struct FuncInfo {
	std::string func_name;
	uint64_t low_pc;
	uint64_t func_size;
};

class LineModification {
public:
	/*the number of a single line*/
	Dwarf_Unsigned lineno;
	/* the memory addresses of this line*/
	std::vector<Dwarf_Addr> line_addresses;
    /* Functions that this line modifies */
	std::vector<FuncInfo> modified_functions;

};

/** Should an instruction that reference something in the data section point
   to the old or the new exe 
*/
enum DataRefLocation{OLD_EXE, NEW_EXE};

class StringDataRef { 
public:
    StringDataRef(std::string str, DataRefLocation location) : string(str), location(location) {}
    std::string string;
    DataRefLocation location;
};

class GlobalVarDataRef { 
public:
    GlobalVarDataRef(Symbol* sym, DataRefLocation location) : symbol(sym), location(location) {}
    Symbol* symbol;
    DataRefLocation location;
};

typedef std::vector<LineModification> LineModificationList;

/**
 * Map is of following format {filename : { 'ins'/'del' : line_num } } 
 */
typedef std::map<std::string, std::map<std::string, std::vector<int>>> FileToLineMap;

typedef std::vector<MuplayBinaryModificationSummary> SummaryList;

typedef std::map<std::string, std::string> StringMap;
typedef std::pair<std::string, std::string> StringPair;

enum ModificationType{INSERTED, DELETED};


} //namespace rr

#endif

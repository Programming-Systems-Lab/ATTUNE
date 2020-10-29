/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <dirent.h>
#include <unistd.h>

#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Command.h"
#include "ElfReader.h"
#include "RecordSession.h"
#include "TraceStream.h"
#include "core.h"
#include "log.h"
#include "main.h"

using namespace std;

namespace rr {

/// Prints JSON containing
/// "relevant_binaries": an array of strings, trace-relative binary file names
/// "symlinks": an array of objects, {"from":<path>, "to":<path>}
/// "files": a map from VCS directory name to array of files relative to that directory
/// An empty VCS directory name means files not under any VCS.
class SourcesCommand : public Command {
public:
  virtual int run(vector<string>& args) override;

protected:
  SourcesCommand(const char* name, const char* help) : Command(name, help) {}

  static SourcesCommand singleton;
};

SourcesCommand SourcesCommand::singleton("sources", " rr sources [<trace_dir>]\n");

static void parent_dir(string& s) {
  size_t p = s.rfind('/');
  if (p == string::npos) {
    s.clear();
  } else if (p > 0) {
    s.resize(p);
  }
}

static void base_name(string& s) {
  size_t p = s.rfind('/');
  if (p != string::npos) {
    s.erase(0, p + 1);
  }
}

static bool is_component(const char* p, const char* component) {
  while (*component) {
    if (*p != *component) {
      return false;
    }
    ++p;
    ++component;
  }
  return *p == '/' || !*p;
}

static void normalize_file_name(string& s) {
  size_t s_len = s.size();
  size_t out = 0;
  for (size_t i = 0; i < s_len; ++i) {
    if (s[i] == '/') {
      if (s.c_str()[i + 1] == '/') {
        // Skip redundant '/'
        continue;
      }
      if (is_component(s.c_str() + i + 1, ".")) {
        // Skip redundant '/.'
        ++i;
        continue;
      }
      if (is_component(s.c_str() + i + 1, "..")) {
        // Peel off '/..'
        size_t p = s.rfind('/', out - 1);
        if (p != string::npos) {
          out = p;
          i += 2;
          continue;
        }
      }
    }
    s[out] = s[i];
    ++out;
  }
  s.resize(out);
}

// file_name cannot be null, but the others can be.
static void resolve_file_name(const char* original_file_dir,
                              const char* comp_dir, const char* rel_dir,
                              const char* file_name, set<string>* file_names) {
  const char* names[] = { original_file_dir, comp_dir, rel_dir, file_name };
  ssize_t first_absolute = -1;
  for (ssize_t i = 0; i < 4; ++i) {
    if (names[i] && names[i][0] == '/') {
      first_absolute = i;
    }
  }
  string s = first_absolute >= 0 ? "" : "/";
  for (size_t i = (first_absolute >= 0 ? first_absolute : 0); i < 4; ++i) {
    if (!names[i]) {
      continue;
    }
    if (!s.empty() && s.back() != '/') {
      s.push_back('/');
    }
    s += names[i];
  }
  file_names->insert(move(s));
}

static bool list_source_files(ElfFileReader& reader, const string& original_file_name,
                              set<string>* file_names) {
  DwarfSpan debug_info = reader.dwarf_section(".debug_info");
  DwarfSpan debug_abbrev = reader.dwarf_section(".debug_abbrev");
  DwarfSpan debug_str = reader.dwarf_section(".debug_str");
  DwarfSpan debug_line = reader.dwarf_section(".debug_line");
  if (debug_info.empty() || debug_abbrev.empty() ||
      debug_str.empty() || debug_line.empty())  {
    return false;
  }

  string original_file_dir = original_file_name;
  parent_dir(original_file_dir);

  DwarfAbbrevs abbrevs(debug_abbrev);
  do {
    bool ok = true;
    DwarfCompilationUnit cu = DwarfCompilationUnit::next(&debug_info, abbrevs, &ok);
    if (!ok) {
      break;
    }
    const char* comp_dir = cu.die().string_attr(DW_AT_comp_dir, debug_str, &ok);
    if (!ok) {
      continue;
    }
    const char* source_file_name = cu.die().string_attr(DW_AT_name, debug_str, &ok);
    if (!ok) {
      continue;
    }
    if (source_file_name) {
      resolve_file_name(original_file_dir.c_str(), comp_dir, nullptr, source_file_name, file_names);
    }
    intptr_t stmt_list = cu.die().lineptr_attr(DW_AT_stmt_list, &ok);
    if (stmt_list < 0 || !ok) {
      continue;
    }
    DwarfLineNumberTable lines(debug_line.subspan(stmt_list), &ok);
    if (!ok) {
      continue;
    }
    for (auto& f : lines.file_names()) {
      if (!f.file_name) {
        // Already resolved above.
        continue;
      }
      const char* dir = lines.directories()[f.directory_index];
      resolve_file_name(original_file_dir.c_str(), comp_dir, dir, f.file_name, file_names);
    }
  } while (!debug_info.empty());

  return true;
}

struct Symlink {
  string from;
  string to;
};

static bool has_subdir(string& base, const char* suffix) {
  base += suffix;
  int ret = access(base.c_str(), F_OK);
  base.resize(base.size() - strlen(suffix));
  return !ret;
}

static void check_vcs_root(string& path, set<string>* vcs_dirs) {
  if (has_subdir(path, "/.git") || has_subdir(path, "/.hg")) {
    vcs_dirs->insert(path + "/");
  }
}

// Returns an empty string if the path does not exist or
// is not accessible.
// `path` need not be normalized, i.e. may contain .. or .
// components.
// The result string, if non-empty, will be absolute,
// normalized, and contain no symlink components.
// The keys in resolved_dirs are absolute file paths
// that may contain symlink components and need not be
// normalized.
// The values in resolved_dirs are always absolute, normalized,
// contain no symlink components, and are directories.
static string resolve_symlinks(const string& path,
                               bool is_file,
                               unordered_map<string, string>* resolved_dirs,
                               vector<Symlink>* symlinks,
                               set<string>* vcs_dirs) {
  // Absolute, not normalized. We don't keep this normalized because
  // we want resolved_dirs to work well.
  // This is always a prefix of `path`.
  string base = path;
  // Absolute, normalized, no symlink components.
  string resolved_base;
  string rest;
  while (true) {
    size_t p = base.rfind('/');
    if (p == 0) {
      base = "";
      rest = path;
      break;
    }
    base.resize(p - 1);
    auto it = resolved_dirs->find(base);
    if (it != resolved_dirs->end()) {
      resolved_base = it->second;
      rest = path.substr(p);
      break;
    }
  }
  // Now iterate through the components of "rest".
  // p points to some '/'-starting component in `rest`.
  size_t p = 0;
  while (true) {
    size_t next = rest.find('/', p + 1);
    bool base_is_file = false;
    size_t end;
    if (next == string::npos) {
      base.append(rest, p, rest.size() - p);
      resolved_base.append(rest, p, rest.size() - p);
      base_is_file = is_file;
      end = rest.size();
    } else {
      base.append(rest, p, next - p);
      resolved_base.append(rest, p, next - p);
      end = next;
    }

    if ((end == p + 2 && memcmp(rest.c_str() + p, "/.", 2) == 0) ||
        (end == p + 3 && memcmp(rest.c_str() + p, "/..", 3) == 0)) {
      normalize_file_name(resolved_base);
    }

    p = next;

    // Now make resolved_base actually resolved.
    // First see if our new resolved_base is cached.
    auto it = resolved_dirs->find(resolved_base);
    if (it != resolved_dirs->end()) {
      resolved_base = it->second;
      if (next == string::npos) {
        return resolved_base;
      }
      resolved_dirs->insert(make_pair(base, resolved_base));
      continue;
    }

    char buf[PATH_MAX + 1];
    ssize_t ret = readlink(resolved_base.c_str(), buf, sizeof(buf));
    if (ret >= 0) {
      buf[ret] = 0;
      string target;
      if (buf[0] != '/') {
        target = base;
        parent_dir(target);
        if (target.size() > 1) {
          target.push_back('/');
        }
      }
      target += buf;
      // We can't normalize `target` because `buf` may itself contain
      // unresolved symlinks, which make normalization non-semantics-preserving.
      string resolved = resolve_symlinks(target, base_is_file, resolved_dirs, symlinks, vcs_dirs);
      symlinks->push_back({ resolved_base, resolved });
      if (!base_is_file) {
        check_vcs_root(resolved, vcs_dirs);
        // Cache the result of the readlink operation
        resolved_dirs->insert(make_pair(move(resolved_base), resolved));
        // And cache based on the original `base`.
        resolved_dirs->insert(make_pair(base, resolved));
      }
      resolved_base = resolved;
      if (next == string::npos) {
        return resolved_base;
      }
    } else {
      if (errno == ENOENT || errno == EACCES || errno == ENOTDIR) {
        // Path is invalid
        resolved_base.clear();
        return resolved_base;
      }
      if (errno != EINVAL) {
        FATAL() << "Failed to readlink " << base;
      }
      if (!base_is_file) {
        check_vcs_root(resolved_base, vcs_dirs);
        // Cache the result of the readlink operation
        resolved_dirs->insert(make_pair(resolved_base, resolved_base));
        // And cache based on the original `base`.
        resolved_dirs->insert(make_pair(base, resolved_base));
      }
      if (next == string::npos) {
        return resolved_base;
      }
    }
  }
}

/// Adds to vcs_dirs any directory paths under any
/// of our resolved directories.
static void build_symlink_map(const set<string>& file_names,
                              set<string>* resolved_file_names,
                              vector<Symlink>* symlinks,
                              set<string>* vcs_dirs) {
  // <dir> -> <path> --- <dir> resolves to <path> using the
  // current value of `symlinks` (and <path> contains no symlinks).
  // If <path> is the empty string then that means the same as <dir>.
  unordered_map<string, string> resolved_dirs;
  for (auto& file_name : file_names) {
    string resolved = resolve_symlinks(file_name, true, &resolved_dirs, symlinks, vcs_dirs);
    if (resolved.empty()) {
      LOG(info) << "File " << file_name << " not found, skipping";
    } else {
      LOG(debug) << "File " << file_name << " resolved to " << resolved;
      resolved_file_names->insert(resolved);
    }
  }
}

static string json_escape(const string& str, size_t pos = 0) {
  string out;
  for (size_t i = pos; i < str.size(); ++i) {
    char c = str[i];
    if (c < 32) {
      char buf[8];
      sprintf(buf, "\\u%04x", c);
      out += c;
    } else if (c == '\\') {
      out += "\\\\";
    } else if (c == '\"') {
      out += "\\\"";
    } else {
      out += c;
    }
  }
  return out;
}

static bool starts_with(const string& s, const string& prefix) {
  return strncmp(s.c_str(), prefix.c_str(), prefix.size()) == 0;
}

static int sources(const string& trace_dir) {
  TraceReader trace(trace_dir);
  DIR* files = opendir(trace.dir().c_str());
  if (!files) {
    FATAL() << "Can't open trace dir";
  }

  map<string, string> binary_file_names;
  while (true) {
    TraceReader::MappedData data;
    bool found;
    KernelMapping km = trace.read_mapped_region(
        &data, &found, TraceReader::VALIDATE, TraceReader::ANY_TIME);
    if (!found) {
      break;
    }
    if (data.source == TraceReader::SOURCE_FILE) {
      binary_file_names.insert(make_pair(move(data.file_name), km.fsname()));
    }
  }

  vector<string> relevant_binary_names;
  set<string> file_names;
  for (auto& pair : binary_file_names) {
    ScopedFd fd(pair.first.c_str(), O_RDONLY);
    if (!fd.is_open()) {
      FATAL() << "Can't open " << pair.first;
    }
    LOG(info) << "Examining " << pair.first;
    ElfFileReader reader(fd);
    if (!reader.ok()) {
      LOG(info) << "Probably not an ELF file, skipping";
      continue;
    }
    if (list_source_files(reader, pair.second, &file_names)) {
      string name = pair.first;
      base_name(name);
      relevant_binary_names.push_back(move(name));
    } else {
      LOG(info) << "No debuginfo found";
    }
  }

  set<string> resolved_file_names;
  vector<Symlink> symlinks;
  set<string> vcs_dirs;
  build_symlink_map(file_names, &resolved_file_names, &symlinks, &vcs_dirs);
  file_names.clear();

  map<string, vector<const string*>> vcs_files;
  vector<const string*> vcs_stack;
  vector<const string*> vcs_dirs_vector;
  auto vcs_dir_iterator = vcs_dirs.begin();
  for (auto& f : resolved_file_names) {
    while (!vcs_stack.empty() && !starts_with(f, *vcs_stack.back())) {
      vcs_stack.pop_back();
    }
    while (vcs_dir_iterator != vcs_dirs.end() && starts_with(f, *vcs_dir_iterator)) {
      vcs_stack.push_back(&*vcs_dir_iterator);
      vcs_dirs_vector.push_back(&*vcs_dir_iterator);
      ++vcs_dir_iterator;
    }
    if (vcs_stack.empty()) {
      vcs_files[string()].push_back(&f);
    } else {
      vcs_files[*vcs_stack.back()].push_back(&f);
    }
  }

  printf("{\n");
  printf("  \"relevant_binaries\":[\n");
  for (size_t i = 0; i < relevant_binary_names.size(); ++i) {
    printf("    \"%s\"%s\n", json_escape(relevant_binary_names[i]).c_str(),
           i == relevant_binary_names.size() - 1 ? "" : ",");
  }
  printf("  ],\n");
  printf("  \"symlinks\":[\n");
  for (size_t i = 0; i < symlinks.size(); ++i) {
    auto& link = symlinks[i];
    printf("    { \"from\":\"%s\", \"to\":\"%s\" }%s\n",
           json_escape(link.from).c_str(),
           json_escape(link.to).c_str(),
           i == symlinks.size() - 1 ? "" : ",");
  }
  printf("  ],\n");
  printf("  \"files\":{\n");
  for (size_t i = 0; i < vcs_dirs_vector.size(); ++i) {
    auto& dir = *vcs_dirs_vector[i];
    string path = json_escape(dir);
    if (path.size() > 1) {
      // Pop final '/'
      path.pop_back();
    }
    printf("    \"%s\": [\n", path.c_str());
    auto& files = vcs_files[dir];
    for (size_t j = 0; j < files.size(); ++j) {
      printf("      \"%s\"%s\n", json_escape(*files[j], dir.size()).c_str(),
             j == files.size() - 1 ? "" : ",");
    }
    printf("    ]%s\n", i == vcs_dirs_vector.size() - 1 ? "" : ",");
  }
  printf("  }\n");
  printf("}\n");

  return 0;
}

int SourcesCommand::run(vector<string>& args) {
  while (parse_global_option(args)) {
  }

  string trace_dir;
  if (!parse_optional_trace_dir(args, &trace_dir)) {
    print_help(stderr);
    return 1;
  }

  return sources(trace_dir);
}

} // namespace rr

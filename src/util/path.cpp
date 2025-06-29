/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "util/path.h"

#include <filesystem>

#include "pl/path.h"
#include "util/algorithm.h"
#include "util/map.h"
#include "util/md5.h"
#include "util/set.h"
#include "util/string.h"
#include "util/vector.h"

#include <cstdio>

#include <sys/stat.h>

#include <zstd.h>

#if defined(_WIN32)
#  define DIR_SEP '\\'
#  define DIR_SEP_ALT '/'
#  include <direct.h>
#else
#  define DIR_SEP '/'
#  include <dirent.h>
#  include <pwd.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#ifdef HAVE_SHLWAPI_H
#  include <shlwapi.h>
#endif

#ifdef _WIN32
#  include "util/windows.h"
#endif

namespace ccl {

#ifdef _WIN32
#  if defined(_MSC_VER) || defined(__MINGW64__)
typedef struct _stat64 path_stat_t;
#  elif defined(__MINGW32__)
typedef struct _stati64 path_stat_t;
#  else
typedef struct _stat path_stat_t;
#  endif
#  ifndef S_ISDIR
#    define S_ISDIR(x) (((x) & _S_IFDIR) == _S_IFDIR)
#  endif
#else
using path_stat_t = struct stat;
#endif

static string cached_path;
static string cached_user_path;
static string cached_xdg_cache_path;

namespace {

#ifdef _WIN32
class directory_iterator {
 public:
  class path_info {
   public:
    path_info(const string &path, const WIN32_FIND_DATAW &find_data)
        : path_(path), find_data_(find_data)
    {
    }

    string path()
    {
      return path_join(path_, string_from_wstring(find_data_.cFileName));
    }

   protected:
    const string &path_;
    const WIN32_FIND_DATAW &find_data_;
  };

  directory_iterator() : path_info_("", find_data_), h_find_(INVALID_HANDLE_VALUE) {}

  explicit directory_iterator(const string &path) : path_(path), path_info_(path, find_data_)
  {
    string wildcard = path;
    if (wildcard[wildcard.size() - 1] != DIR_SEP) {
      wildcard += DIR_SEP;
    }
    wildcard += "*";
    h_find_ = FindFirstFileW(string_to_wstring(wildcard).c_str(), &find_data_);
    if (h_find_ != INVALID_HANDLE_VALUE) {
      skip_dots();
    }
  }

  ~directory_iterator()
  {
    if (h_find_ != INVALID_HANDLE_VALUE) {
      FindClose(h_find_);
    }
  }

  directory_iterator &operator++()
  {
    step();
    return *this;
  }

  path_info *operator->()
  {
    return &path_info_;
  }

  bool operator!=(const directory_iterator &other)
  {
    return h_find_ != other.h_find_;
  }

 protected:
  bool step()
  {
    if (do_step()) {
      return skip_dots();
    }
    return false;
  }

  bool do_step()
  {
    if (h_find_ != INVALID_HANDLE_VALUE) {
      bool result = FindNextFileW(h_find_, &find_data_) == TRUE;
      if (!result) {
        FindClose(h_find_);
        h_find_ = INVALID_HANDLE_VALUE;
      }
      return result;
    }
    return false;
  }

  bool skip_dots()
  {
    while (wcscmp(find_data_.cFileName, L".") == 0 || wcscmp(find_data_.cFileName, L"..") == 0) {
      if (!do_step()) {
        return false;
      }
    }
    return true;
  }

  string path_;
  path_info path_info_;
  WIN32_FIND_DATAW find_data_;
  HANDLE h_find_;
};
#else /* _WIN32 */

class directory_iterator {
 public:
  class path_info {
   public:
    explicit path_info(const string &path) : path_(path) {}

    string path()
    {
      return path_join(path_, entry_->d_name);
    }

    void current_entry_set(const struct dirent *entry)
    {
      entry_ = entry;
    }

   protected:
    const string &path_;
    const struct dirent *entry_ = nullptr;
  };

  directory_iterator() : path_info_(""), name_list_(nullptr), num_entries_(-1), cur_entry_(-1) {}

  explicit directory_iterator(const string &path) : path_(path), path_info_(path_), cur_entry_(0)
  {
    num_entries_ = scandir(path.c_str(), &name_list_, nullptr, alphasort);
    if (num_entries_ < 0) {
      perror("scandir");
    }
    else {
      skip_dots();
    }
  }

  ~directory_iterator()
  {
    destroy_name_list();
  }

  directory_iterator &operator++()
  {
    step();
    return *this;
  }

  path_info *operator->()
  {
    path_info_.current_entry_set(name_list_[cur_entry_]);
    return &path_info_;
  }

  bool operator!=(const directory_iterator &other)
  {
    return name_list_ != other.name_list_;
  }

 protected:
  bool step()
  {
    if (do_step()) {
      return skip_dots();
    }
    return false;
  }

  bool do_step()
  {
    ++cur_entry_;
    if (cur_entry_ >= num_entries_) {
      destroy_name_list();
      return false;
    }
    return true;
  }

  /* Skip . and .. folders. */
  bool skip_dots()
  {
    while (strcmp(name_list_[cur_entry_]->d_name, ".") == 0 ||
           strcmp(name_list_[cur_entry_]->d_name, "..") == 0)
    {
      if (!step()) {
        return false;
      }
    }
    return true;
  }

  void destroy_name_list()
  {
    if (name_list_ == nullptr) {
      return;
    }
    for (int i = 0; i < num_entries_; ++i) {
      free(name_list_[i]);
    }
    free((void *)name_list_);
    name_list_ = nullptr;
  }

  string path_;
  path_info path_info_;
  struct dirent **name_list_;
  int num_entries_, cur_entry_;
};

#endif /* _WIN32 */

size_t find_last_slash(const string &path)
{
  for (size_t i = 0; i < path.size(); ++i) {
    const size_t index = path.size() - 1 - i;
#ifdef _WIN32
    if (path[index] == DIR_SEP || path[index] == DIR_SEP_ALT)
#else
    if (path[index] == DIR_SEP)
#endif
    {
      return index;
    }
  }
  return string::npos;
}

} /* namespace */

static char *path_specials(const string &sub)
{
  static bool env_init = false;
  static char *env_shader_path;
  static char *env_source_path;
  if (!env_init) {
    env_shader_path = getenv("CYCLES_SHADER_PATH");
    /* NOTE: It is KERNEL in env variable for compatibility reasons. */
    env_source_path = getenv("CYCLES_KERNEL_PATH");
    env_init = true;
  }
  if (env_shader_path != nullptr && sub == "shader") {
    return env_shader_path;
  }
  if (env_source_path != nullptr && sub == "source") {
    return env_source_path;
  }
  return nullptr;
}

#if defined(__linux__) || defined(__APPLE__)
static string path_xdg_cache_get()
{
  const char *home = getenv("XDG_CACHE_HOME");
  if (home) {
    return string(home);
  }
  home = getenv("HOME");
  if (home == nullptr) {
    home = getpwuid(getuid())->pw_dir;
  }
  return path_join(string(home), ".cache");
}
#endif

void path_init(const string &path, const string &user_path)
{
  cached_path = path;
  cached_user_path = user_path;
}

string path_get(const string &sub)
{
  char *special = path_specials(sub);
  if (special != nullptr) {
    return special;
  }

  if (cached_path.empty()) {
    cached_path = path_dirname(this_program_path());
  }

  return path_join(cached_path, sub);
}

string path_user_get(const string &sub)
{
  if (cached_user_path.empty()) {
    cached_user_path = path_dirname(this_program_path());
  }

  return path_join(cached_user_path, sub);
}

string path_cache_get(const string &sub)
{
#if defined(__linux__) || defined(__APPLE__)
  if (cached_xdg_cache_path.empty()) {
    cached_xdg_cache_path = path_xdg_cache_get();
  }
  const string result = path_join(cached_xdg_cache_path, "cycles");
  return path_join(result, sub);
#else
  /* TODO(sergey): What that should be on Windows? */
  return path_user_get(path_join("cache", sub));
#endif
}

#if defined(__linux__) || defined(__APPLE__)
string path_xdg_home_get(const string &sub = "");
#endif

string path_filename(const string &path)
{
  const size_t index = find_last_slash(path);
  if (index != string::npos) {
    /* Corner cases to match boost behavior. */
#ifndef _WIN32
    if (index == 0 && path.size() == 1) {
      return path;
    }
#endif
    if (index == path.size() - 1) {
#ifdef _WIN32
      if (index == 2) {
        return string(1, DIR_SEP);
      }
#endif
      return ".";
    }
    return path.substr(index + 1, path.size() - index - 1);
  }
  return path;
}

string path_dirname(const string &path)
{
  const size_t index = find_last_slash(path);
  if (index != string::npos) {
#ifndef _WIN32
    if (index == 0 && path.size() > 1) {
      return string(1, DIR_SEP);
    }
#endif
    return path.substr(0, index);
  }
  return "";
}

string path_join(const string &dir, const string &file)
{
  if (dir.empty()) {
    return file;
  }
  if (file.empty()) {
    return dir;
  }
  string result = dir;
#ifndef _WIN32
  if (result[result.size() - 1] != DIR_SEP && file[0] != DIR_SEP)
#else
  if (result[result.size() - 1] != DIR_SEP && result[result.size() - 1] != DIR_SEP_ALT &&
      file[0] != DIR_SEP && file[0] != DIR_SEP_ALT)
#endif
  {
    result += DIR_SEP;
  }
  result += file;
  return result;
}

string path_escape(const string &path)
{
  string result = path;
  string_replace(result, " ", "\\ ");
  return result;
}

bool path_is_relative(const string &path)
{
#ifdef _WIN32
#  ifdef HAVE_SHLWAPI_H
  return PathIsRelative(path.c_str());
#  else  /* HAVE_SHLWAPI_H */
  if (path.size() >= 3) {
    return !(((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) &&
             path[1] == ':' && path[2] == DIR_SEP);
  }
  return true;
#  endif /* HAVE_SHLWAPI_H */
#else    /* _WIN32 */
  if (path.empty()) {
    return true;
  }
  return path[0] != DIR_SEP;
#endif   /* _WIN32 */
}

#ifdef _WIN32
/* Add a slash if the UNC path points to a share. */
static string path_unc_add_slash_to_share(const string &path)
{
  size_t slash_after_server = path.find(DIR_SEP, 2);
  if (slash_after_server != string::npos) {
    size_t slash_after_share = path.find(DIR_SEP, slash_after_server + 1);
    if (slash_after_share == string::npos) {
      return path + DIR_SEP;
    }
  }
  return path;
}

/* Convert:
 *    \\?\UNC\server\share\folder\... to \\server\share\folder\...
 *    \\?\C:\ to C:\ and \\?\C:\folder\... to C:\folder\...
 */
static string path_unc_to_short(const string &path)
{
  size_t len = path.size();
  if ((len > 3) && (path[0] == DIR_SEP) && (path[1] == DIR_SEP) && (path[2] == '?') &&
      ((path[3] == DIR_SEP) || (path[3] == DIR_SEP_ALT)))
  {
    if ((len > 5) && (path[5] == ':')) {
      return path.substr(4, len - 4);
    }
    else if ((len > 7) && (path.substr(4, 3) == "UNC") &&
             ((path[7] == DIR_SEP) || (path[7] == DIR_SEP_ALT)))
    {
      return "\\\\" + path.substr(8, len - 8);
    }
  }
  return path;
}

static string path_cleanup_unc(const string &path)
{
  string result = path_unc_to_short(path);
  if (path.size() > 2) {
    /* It's possible path is now a non-UNC. */
    if (result[0] == DIR_SEP && result[1] == DIR_SEP) {
      return path_unc_add_slash_to_share(result);
    }
  }
  return result;
}

/* Make path compatible for stat() functions. */
static string path_make_compatible(const string &path)
{
  string result = path;
  /* In Windows stat() doesn't recognize dir ending on a slash. */
  if (result.size() > 3 && result[result.size() - 1] == DIR_SEP) {
    result.resize(result.size() - 1);
  }
  /* Clean up UNC path. */
  if ((path.size() >= 3) && (path[0] == DIR_SEP) && (path[1] == DIR_SEP)) {
    result = path_cleanup_unc(result);
  }
  /* Make sure volume-only path ends up wit a directory separator. */
  if (result.size() == 2 && result[1] == ':') {
    result += DIR_SEP;
  }
  return result;
}

static int path_wstat(const wstring &path_wc, path_stat_t *st)
{
#  if defined(_MSC_VER) || defined(__MINGW64__)
  return _wstat64(path_wc.c_str(), st);
#  elif defined(__MINGW32__)
  return _wstati64(path_wc.c_str(), st);
#  else
  return _wstat(path_wc.c_str(), st);
#  endif
}

static int path_stat(const string &path, path_stat_t *st)
{
  wstring path_wc = string_to_wstring(path);
  return path_wstat(path_wc, st);
}
#else  /* _WIN32 */
static int path_stat(const string &path, path_stat_t *st)
{
  return stat(path.c_str(), st);
}
#endif /* _WIN32 */

size_t path_file_size(const string &path)
{
  path_stat_t st;
  if (path_stat(path, &st) != 0) {
    return -1;
  }
  return st.st_size;
}

bool path_exists(const string &path)
{
#ifdef _WIN32
  string fixed_path = path_make_compatible(path);
  wstring path_wc = string_to_wstring(fixed_path);
  path_stat_t st;
  if (path_wstat(path_wc, &st) != 0) {
    return false;
  }
  return st.st_mode != 0;
#else  /* _WIN32 */
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  return st.st_mode != 0;
#endif /* _WIN32 */
}

bool path_is_directory(const string &path)
{
  path_stat_t st;
  if (path_stat(path, &st) != 0) {
    return false;
  }
  return S_ISDIR(st.st_mode);
}

static void path_files_md5_hash_recursive(MD5Hash &hash, const string &dir)
{
  if (path_exists(dir)) {
    directory_iterator it(dir);
    const directory_iterator it_end;

    for (; it != it_end; ++it) {
      if (path_is_directory(it->path())) {
        path_files_md5_hash_recursive(hash, it->path());
      }
      else {
        const string filepath = it->path();

        hash.append((const uint8_t *)filepath.c_str(), filepath.size());
        hash.append_file(filepath);
      }
    }
  }
}

string path_files_md5_hash(const string &dir)
{
  /* computes md5 hash of all files in the directory */
  MD5Hash hash;

  path_files_md5_hash_recursive(hash, dir);

  return hash.get_hex();
}

static bool create_directories_recursivey(const string &path)
{
  if (path_is_directory(path)) {
    /* Directory already exists, nothing to do. */
    return true;
  }
  if (path_exists(path)) {
    /* File exists and it's not a directory. */
    return false;
  }

  const string parent = path_dirname(path);
  if (!parent.empty() && parent != path) {
    if (!create_directories_recursivey(parent)) {
      return false;
    }
  }

#ifdef _WIN32
  wstring path_wc = string_to_wstring(path);
  return _wmkdir(path_wc.c_str()) == 0;
#else
  return mkdir(path.c_str(), 0777) == 0;
#endif
}

void path_create_directories(const string &filepath)
{
  const string path = path_dirname(filepath);
  create_directories_recursivey(path);
}

bool path_write_binary(const string &path, const vector<uint8_t> &binary)
{
  path_create_directories(path);

  /* write binary file from memory */
  FILE *f = path_fopen(path, "wb");

  if (!f) {
    return false;
  }

  if (!binary.empty()) {
    fwrite(binary.data(), sizeof(uint8_t), binary.size(), f);
  }

  fclose(f);

  return true;
}

bool path_write_text(const string &path, string &text)
{
  vector<uint8_t> binary(text.length(), 0);
  std::copy(text.begin(), text.end(), binary.begin());

  return path_write_binary(path, binary);
}

bool path_read_binary(const string &path, vector<uint8_t> &binary)
{
  /* read binary file into memory */
  FILE *f = path_fopen(path, "rb");

  if (!f) {
    binary.resize(0);
    return false;
  }

  binary.resize(path_file_size(path));

  if (binary.empty()) {
    fclose(f);
    return false;
  }

  if (fread(binary.data(), sizeof(uint8_t), binary.size(), f) != binary.size()) {
    fclose(f);
    return false;
  }

  fclose(f);

  return true;
}

bool path_read_compressed_binary(const string &path, vector<uint8_t> &binary)
{
  if (!string_endswith(path, ".zst")) {
    return path_read_binary(path, binary);
  }

  vector<uint8_t> compressed;
  if (!path_read_binary(path, compressed)) {
    return false;
  }

  const size_t full_size = ZSTD_getFrameContentSize(compressed.data(), compressed.size());

  if (full_size == ZSTD_CONTENTSIZE_ERROR) {
    /* Potentially corrupted file? */
    return false;
  }
  if (full_size == ZSTD_CONTENTSIZE_UNKNOWN) {
    /* Technically this is an optional field, but we can expect it to be set for now.
     * Otherwise we'd need streaming decompression and repeated resizing of the vector. */
    return false;
  }

  binary.resize(full_size);

  const size_t err = ZSTD_decompress(
      binary.data(), binary.size(), compressed.data(), compressed.size());

  return ZSTD_isError(err) == 0;
}

bool path_read_text(const string &path, string &text)
{
  vector<uint8_t> binary;

  if (!path_exists(path) || !path_read_binary(path, binary)) {
    return false;
  }

  const char *str = (const char *)binary.data();
  const size_t size = binary.size();
  text = string(str, size);

  return true;
}

bool path_read_compressed_text(const string &path, string &text)
{
  vector<uint8_t> binary;

  if (!path_exists(path) || !path_read_compressed_binary(path, binary)) {
    return false;
  }

  const char *str = (const char *)binary.data();
  const size_t size = binary.size();
  text = string(str, size);

  return true;
}

uint64_t path_modified_time(const string &path)
{
  path_stat_t st;
  if (path_stat(path, &st) != 0) {
    return 0;
  }
  return st.st_mtime;
}

bool path_remove(const string &path)
{
  return remove(path.c_str()) == 0;
}

struct SourceReplaceState {
  using ProcessedMapping = map<string, string>;
  /* Base director for all relative include headers. */
  string base;
  /* Result of processed files. */
  ProcessedMapping processed_files;
  /* Set of files containing #pragma once which have been included. */
  set<string> pragma_onced;
};

static string path_source_replace_includes_recursive(const string &source,
                                                     const string &source_filepath,
                                                     SourceReplaceState *state);

static string line_directive(const SourceReplaceState &state,
                             const string &path,
                             const size_t line_number)
{
  string unescaped_path = path;
  /* First we make path relative. */
  if (string_startswith(unescaped_path, state.base)) {
    const string base_file = path_filename(state.base);
    const size_t base_len = state.base.length();
    unescaped_path = base_file +
                     unescaped_path.substr(base_len, unescaped_path.length() - base_len);
  }
  /* Second, we replace all unsafe characters. */
  const size_t length = unescaped_path.length();
  string escaped_path;
  for (size_t i = 0; i < length; ++i) {
    const char ch = unescaped_path[i];
    if (strchr("\"\'\?\\", ch) != nullptr) {
      escaped_path += "\\";
    }
    escaped_path += ch;
  }
  return "#line " + std::to_string(line_number) + '"' + escaped_path + '"';
}

static string path_source_handle_preprocessor(const string &preprocessor_line,
                                              const string &source_filepath,
                                              const size_t line_number,
                                              SourceReplaceState *state)
{
  string result = preprocessor_line;

  string rest_of_line = string_strip(preprocessor_line.substr(1));

  if (0 == strncmp(rest_of_line.c_str(), "include", 7)) {
    rest_of_line = string_strip(rest_of_line.substr(8));
    if (rest_of_line[0] == '"') {
      const size_t n_start = 1;
      const size_t n_end = rest_of_line.find("\"", n_start);
      const string filename = rest_of_line.substr(n_start, n_end - n_start);

      string filepath = path_join(state->base, filename);
      if (!path_exists(filepath)) {
        filepath = path_join(path_dirname(source_filepath), filename);
      }
      string text;
      if (path_read_text(filepath, text)) {
        text = path_source_replace_includes_recursive(text, filepath, state);
        /* Use line directives for better error messages. */
        result = line_directive(*state, filepath, 1) + "\n" + text + "\n" +
                 line_directive(*state, source_filepath, line_number + 1);
      }
    }
  }

  return result;
}

/* Our own little c preprocessor that replaces #includes with the file
 * contents, to work around issue of OpenCL drivers not supporting
 * include paths with spaces in them.
 */
static string path_source_replace_includes_recursive(const string &_source,
                                                     const string &source_filepath,
                                                     SourceReplaceState *state)
{
  const string *psource = &_source;
  string source_new;

  auto pragma_once = _source.find("#pragma once");
  if (pragma_once != string::npos) {
    if (state->pragma_onced.find(source_filepath) != state->pragma_onced.end()) {
      return "";
    }
    state->pragma_onced.insert(source_filepath);

    //                                 "#pragma once"
    //                                 "//prgma once"
    source_new = _source;
    memcpy(source_new.data() + pragma_once, "//pr", 4);
    psource = &source_new;
  }

  /* Try to re-use processed file without spending time on replacing all
   * include directives again.
   */
  const SourceReplaceState::ProcessedMapping::iterator replaced_file = state->processed_files.find(
      source_filepath);
  if (replaced_file != state->processed_files.end()) {
    return replaced_file->second;
  }

  const string &source = *psource;

  /* Perform full file processing. */
  string result;
  const size_t source_length = source.length();
  size_t index = 0;
  /* Information about where we are in the source. */
  size_t line_number = 0;
  size_t column_number = 1;
  /* Currently gathered non-preprocessor token.
   * Store as start/length rather than token itself to avoid overhead of
   * memory re-allocations on each character concatenation.
   */
  size_t token_start = 0;
  size_t token_length = 0;
  /* Denotes whether we're inside of preprocessor line, together with
   * preprocessor line itself.
   *
   * TODO(sergey): Investigate whether using token start/end position
   * gives measurable speedup.
   */
  bool inside_preprocessor = false;
  string preprocessor_line;
  /* Actual loop over the whole source. */
  while (index < source_length) {
    const char ch = source[index];

    if (ch == '\n') {
      if (inside_preprocessor) {
        const string block = path_source_handle_preprocessor(
            preprocessor_line, source_filepath, line_number, state);

        if (!block.empty()) {
          result += block;
        }

        /* Start gathering net part of the token. */
        token_start = index;
        token_length = 0;
        inside_preprocessor = false;
        preprocessor_line = "";
      }
      column_number = 0;
      ++line_number;
    }
    else if (ch == '#' && column_number == 1 && !inside_preprocessor) {
      /* Append all possible non-preprocessor token to the result. */
      if (token_length != 0) {
        result.append(source, token_start, token_length);
        token_start = index;
        token_length = 0;
      }
      inside_preprocessor = true;
    }

    if (inside_preprocessor) {
      preprocessor_line += ch;
    }
    else {
      ++token_length;
    }
    ++index;
    ++column_number;
  }
  /* Append possible tokens which happened before special events handled
   * above.
   */
  if (token_length != 0) {
    result.append(source, token_start, token_length);
  }
  if (inside_preprocessor) {
    result += path_source_handle_preprocessor(
        preprocessor_line, source_filepath, line_number, state);
  }
  /* Store result for further reuse. */
  state->processed_files[source_filepath] = result;
  return result;
}

string path_source_replace_includes(const string &source, const string &path)
{
  SourceReplaceState state;
  state.base = path;
  return path_source_replace_includes_recursive(source, path, &state);
}

FILE *path_fopen(const string &path, const string &mode)
{
#ifdef _WIN32
  wstring path_wc = string_to_wstring(path);
  wstring mode_wc = string_to_wstring(mode);
  return _wfopen(path_wc.c_str(), mode_wc.c_str());
#else
  return fopen(path.c_str(), mode.c_str());
#endif
}

/* LRU Cache for Kernels */

static void path_cache_kernel_mark_used(const string &path)
{
  auto now = std::filesystem::file_time_type::clock::now();
  std::filesystem::last_write_time(path, now);
}

bool path_cache_kernel_exists_and_mark_used(const string &path)
{
  if (path_exists(path)) {
    path_cache_kernel_mark_used(path);
    return true;
  }
  return false;
}

void path_cache_kernel_mark_added_and_clear_old(const string &new_path,
                                                const size_t max_old_kernel_of_same_type)
{
  path_cache_kernel_mark_used(new_path);

  const string dir = path_dirname(new_path);
  if (!path_exists(dir)) {
    return;
  }

  /* Remove older kernels within the same directory. */
  directory_iterator it(dir);
  const directory_iterator it_end;
  vector<pair<std::filesystem::file_time_type, string>> same_kernel_types;

  for (; it != it_end; ++it) {
    const string &path = it->path();
    if (path == new_path) {
      continue;
    }

    std::filesystem::file_time_type last_time = std::filesystem::last_write_time(path);
    same_kernel_types.emplace_back(last_time, path);
  }

  if (same_kernel_types.size() > max_old_kernel_of_same_type) {
    sort(same_kernel_types.begin(), same_kernel_types.end());

    for (int i = 0; i < same_kernel_types.size() - max_old_kernel_of_same_type; i++) {
      path_remove(same_kernel_types[i].second);
    }
  }
}

}

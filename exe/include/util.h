#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef _WIN32
#define MAX_PATH_LENGTH 260
#else
#define MAX_PATH_LENGTH 4096
#endif

#include <wchar.h>
#include <stdint.h>

typedef uint8_t byte;
typedef uint8_t boolean;
typedef char* string;
typedef wchar_t* wstring;
typedef void (*ReaddirCallback)(string, boolean, void*);

#define NO ((boolean)0);
#define YES ((boolean)1);

#ifdef __cplusplus
extern "C" {
#endif

int32_t fs_readdir(string, ReaddirCallback, void*);
string replace_sep(string, string);
boolean fs_copy(string, string);
boolean fs_remove(string);
boolean fs_exists(string);
boolean fs_is_dir(string);
boolean fs_mkdirs(string);
string path_dirname(string);
string path_basename(string);
string path_join(string, string);

boolean apply_patch(string tmpDir, string targetDir);

#ifdef _WIN32
int32_t utf8_acp(string, string, int32_t);
boolean fs_is_dirw(wstring);

boolean run_executable(string);
int32_t start(int argc, wchar_t** argv);
#else
boolean run_executable(string, string*);
int32_t start(int argc, char** argv);
#endif

#ifdef __cplusplus
}
#endif

#endif

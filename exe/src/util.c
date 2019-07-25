#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

static boolean fs_copy_file(string, string);
static boolean fs_copy_dir(string, string);
static void fs_copy_dir_callback(string, boolean, void*);

static boolean fs_remove_file(string);
static boolean fs_remove_dir(string);
static void fs_remove_dir_callback(string, boolean, void*);

#define BUFFER_SIZE 128 * 1024

#ifdef _WIN32
int32_t utf8_acp(string input, string output, int32_t size) {
  int32_t strlength;
  wchar_t* wstr;
  int32_t res;
  strlength = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
  wstr = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wstr == NULL) {
    return -1;
  }
  memset(wstr, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, input, -1, wstr, strlength);

  if (size == 0) {
    strlength = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    free(wstr);
    return strlength;
  }
  if (output == NULL) {
    free(wstr);
    return -1;
  }

  memset(output, 0, size);
  res = WideCharToMultiByte(CP_ACP, 0, wstr, -1, output, size, NULL, NULL);
  free(wstr);
  return res;
}

boolean fs_is_dirw(wstring path) {
  struct _stat info;
  uint32_t i;
  int32_t res;
  wchar_t newPath[MAX_PATH];

  memset(newPath, 0, MAX_PATH * sizeof(wchar_t));
  wcscpy(newPath, path);
  for (i = 0; i < wcslen(newPath); i++) {
    if (newPath[i] == L'/') {
      newPath[i] = L'\\';
    }
  }
  res = _wstat(newPath, &info);
  if (res != 0) {
    return NO;
  }
  return S_IFDIR == (info.st_mode & S_IFDIR);
}
#endif

string path_basename(string path) {
  string sepwin, seppo, sep;

  sepwin = strrchr(path, '\\');
  seppo = strrchr(path, '/');

  if (sepwin == NULL && seppo == NULL) {
    return path;
  }

  if (sepwin != NULL && seppo == NULL) {
    return sepwin + 1;
  }

  if (sepwin == NULL && seppo != NULL) {
    return seppo + 1;
  }

  sep = sepwin > seppo ? sepwin : seppo;

  return sep + 1;
}

string path_join(string a, string b) {
  uint32_t len;
  len = strlen(a);
#ifdef _WIN32
  if (a[len - 1] != '\\') {
    strcat(a, "\\");
  }
  strcat(a, b);
#else
  if (a[len - 1] != '/') {
    strcat(a, "/");
  }
  strcat(a, b);
#endif
  return a;
}

string path_dirname(string path) {
  uint32_t i;
  string sep;
  for (i = 0; i < strlen(path); i++) {
#ifdef _WIN32
    if (path[i] == '/') {
      path[i] = '\\';
    }
#else
    if (path[i] == '\\') {
      path[i] = '/';
    }
#endif
  }

#ifdef _WIN32
  sep = strrchr(path, '\\');
#else
  sep = strrchr(path, '/');
#endif
  if (sep == NULL) {
    path[0] = '.';
    path[1] = '\0';
    return path;
  }

  if (sep == path) {
    path[1] = '\0';
    return path;
  }

  *sep = '\0';
  return path;
}

boolean fs_exists(string path) {
#ifdef _WIN32
  struct _stat info;
  wchar_t* wstr;
#endif
  int32_t res;
  string newPath;
  uint32_t strlength;
  newPath = NULL;
  strlength = strlen(path);
  newPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newPath == NULL) {
    return NO;
  }
  memset(newPath, 0, (strlength + 1));
  replace_sep(path, newPath);

#ifdef _WIN32
  strlength = MultiByteToWideChar(CP_UTF8, 0, newPath, -1, NULL, 0);
  wstr = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wstr == NULL) {
    free(newPath);
    return NO;
  }
  memset(wstr, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, newPath, -1, wstr, strlength);
  res = _wstat(wstr, &info);
  free(wstr);
  free(newPath);
  if (res != 0) {
    return (boolean)(ENOENT != errno);
  }
  return YES;
#else
  res = (access(newPath, F_OK) == 0);
  free(newPath);
  return res;
#endif
}

boolean fs_is_dir(string path) {
#ifdef _WIN32
  struct _stat info;
  wchar_t* wstr;
#else
  struct stat info;
#endif
  int32_t res;
  string newPath;
  uint32_t strlength;
  newPath = NULL;
  strlength = strlen(path);
  newPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newPath == NULL) {
    return NO;
  }
  memset(newPath, 0, (strlength + 1));
  replace_sep(path, newPath);

#ifdef _WIN32
  strlength = MultiByteToWideChar(CP_UTF8, 0, newPath, -1, NULL, 0);
  wstr = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wstr == NULL) {
    free(newPath);
    return NO;
  }
  memset(wstr, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, newPath, -1, wstr, strlength);
  res = _wstat(wstr, &info);
  free(wstr);
  free(newPath);
  if (res != 0) {
    return NO;
  }
  return S_IFDIR == (info.st_mode & S_IFDIR);
#else
  res = stat(newPath, &info);
  free(newPath);
  if (res != 0) {
    return NO;
  }
  return S_IFDIR == (info.st_mode & S_IFDIR);
#endif
}

typedef struct FsWalkDirData {
  string src;
  string dest;
  boolean res;
} FsWalkDirData;

void fs_copy_dir_callback(string name, boolean isDir, void* userData) {
  FsWalkDirData* data;
  string fullRoot, fullDest;
  data = (FsWalkDirData*)userData;

  fullRoot = (string)malloc(MAX_PATH_LENGTH * sizeof(char));
  if (fullRoot == NULL) {
    data->res = NO;
    return;
  }
  memset(fullRoot, 0, MAX_PATH_LENGTH * sizeof(char));
  strcpy(fullRoot, data->src);
  path_join(fullRoot, name);

  fullDest = (string)malloc(MAX_PATH_LENGTH * sizeof(char));
  if (fullDest == NULL) {
    free(fullRoot);
    data->res = NO;
    return;
  }
  memset(fullDest, 0, MAX_PATH_LENGTH * sizeof(char));
  strcpy(fullDest, data->dest);
  path_join(fullDest, name);

  if (isDir) {
    if (!fs_copy_dir(fullRoot, fullDest)) {
      data->res = NO;
    }
  } else {
    if (!fs_copy_file(fullRoot, fullDest)) {
      data->res = NO;
    }
  }

  free(fullRoot);
  free(fullDest);
}

boolean fs_copy_dir(string src, string dest) {
  string newSrcPath, newDestPath;
  uint32_t strlength;
  FsWalkDirData userData;

  strlength = strlen(src);
  newSrcPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newSrcPath == NULL) {
    return NO;
  }
  memset(newSrcPath, 0, (strlength + 1));
  replace_sep(src, newSrcPath);

  strlength = strlen(dest);
  newDestPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newDestPath == NULL) {
    free(newSrcPath);
    return NO;
  }
  memset(newDestPath, 0, (strlength + 1));
  replace_sep(dest, newDestPath);

  fs_mkdirs(newDestPath);

  userData.dest = newDestPath;
  userData.src = newSrcPath;
  userData.res = YES;
  
  if (-1 == fs_readdir(src, fs_copy_dir_callback, &userData)) {
    free(newSrcPath);
    free(newDestPath);
    return NO;
  }

  free(newSrcPath);
  free(newDestPath);
  return userData.res;
}

boolean fs_copy_file(string src, string dest) {
#ifdef _WIN32
  wchar_t* wsrc;
  wchar_t* wdest;
#endif
  string newSrcPath, newDestPath, destDir;
  uint32_t strlength;
  FILE *srcfp, *destfp;
  byte* buf;
  uint32_t read;

  strlength = strlen(src);
  newSrcPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newSrcPath == NULL) {
    return NO;
  }
  memset(newSrcPath, 0, (strlength + 1));
  replace_sep(src, newSrcPath);

  strlength = strlen(dest);
  newDestPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newDestPath == NULL) {
    free(newSrcPath);
    return NO;
  }
  memset(newDestPath, 0, (strlength + 1));
  replace_sep(dest, newDestPath);

#ifdef _WIN32
  strlength = MultiByteToWideChar(CP_UTF8, 0, newSrcPath, -1, NULL, 0);
  wsrc = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wsrc == NULL) {
    free(newSrcPath);
    free(newDestPath);
    return NO;
  }
  memset(wsrc, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, newSrcPath, -1, wsrc, strlength);
  srcfp = _wfopen(wsrc, L"rb+");
  free(wsrc);
#else
  srcfp = fopen(newSrcPath, "rb+");
#endif

  if (srcfp == NULL) {
    free(newSrcPath);
    free(newDestPath);
    return NO;
  }

  destDir = (string)malloc((strlen(newDestPath) + 1) * sizeof(char));
  if (destDir == NULL) {
    free(newSrcPath);
    free(newDestPath);
    return NO;
  }
  memset(destDir, 0, (strlen(newDestPath) + 1) * sizeof(char));
  strcpy(destDir, newDestPath);
  path_dirname(destDir);
  fs_mkdirs(destDir);
  free(destDir);

#ifdef _WIN32
  strlength = MultiByteToWideChar(CP_UTF8, 0, newDestPath, -1, NULL, 0);
  wdest = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wdest == NULL) {
    free(newSrcPath);
    free(newDestPath);
    fclose(srcfp);
    return NO;
  }
  memset(wdest, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, newDestPath, -1, wdest, strlength);
  destfp = _wfopen(wdest, L"wb+");
  free(wdest);
#else
  destfp = fopen(newDestPath, "wb+");
#endif

  if (destfp == NULL) {
    free(newSrcPath);
    free(newDestPath);
    fclose(srcfp);
    return NO;
  }

  buf = (byte*)malloc(BUFFER_SIZE * sizeof(byte));
  if (buf == NULL) {
    free(newSrcPath);
    free(newDestPath);
    fclose(srcfp);
    fclose(destfp);
    return NO;
  }
  
  while ((read = (uint32_t)fread(buf, sizeof(byte), BUFFER_SIZE, srcfp)) > 0) {
    fwrite(buf, sizeof(byte), read, destfp);
  }
  free(newSrcPath);
  free(newDestPath);
  fclose(srcfp);
  fclose(destfp);
  free(buf);
  return YES;
}

boolean fs_copy(string src, string dest) {
  if (fs_is_dir(src)) {
    return fs_copy_dir(src, dest);
  }
  return fs_copy_file(src, dest);
}

boolean fs_remove(string src) {
  if (fs_is_dir(src)) {
    return fs_remove_dir(src);
  }
  return fs_remove_file(src);
}

boolean fs_remove_file(string src) {
#ifdef _WIN32
  wchar_t* wsrc;
#endif
  string newSrcPath;
  uint32_t strlength;
  int32_t res;

  strlength = strlen(src);
  newSrcPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newSrcPath == NULL) {
    return NO;
  }
  memset(newSrcPath, 0, (strlength + 1));
  replace_sep(src, newSrcPath);

#ifdef _WIN32
  strlength = MultiByteToWideChar(CP_UTF8, 0, newSrcPath, -1, NULL, 0);
  wsrc = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wsrc == NULL) {
    free(newSrcPath);
    return NO;
  }
  memset(wsrc, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, newSrcPath, -1, wsrc, strlength);
  res = _wunlink(wsrc);
  free(newSrcPath);
  free(wsrc);
  return res == 0;
#else
  res = unlink(newSrcPath);
  free(newSrcPath);
  return res == 0;
#endif
}

void fs_remove_dir_callback(string name, boolean isDir, void* userData) {
  FsWalkDirData* data;
  string fullRoot;
  data = (FsWalkDirData*)userData;

  fullRoot = (string)malloc(MAX_PATH_LENGTH * sizeof(char));
  if (fullRoot == NULL) {
    return;
  }
  memset(fullRoot, 0, MAX_PATH_LENGTH * sizeof(char));
  strcpy(fullRoot, data->src);
  path_join(fullRoot, name);

  if (isDir) {
    if (!fs_remove_dir(fullRoot)) {
      data->res = NO;
    }
  } else {
    if (!fs_remove_file(fullRoot)) {
      data->res = NO;
    }
  }

  free(fullRoot);
}

boolean fs_remove_dir(string src) {
#ifdef _WIN32
  wchar_t* wsrc;
#endif
  string newSrcPath;
  uint32_t strlength;
  FsWalkDirData userData;
  int32_t res;

  strlength = strlen(src);
  newSrcPath = (string)malloc((strlength + 1) * sizeof(char));
  if (newSrcPath == NULL) {
    return NO;
  }
  memset(newSrcPath, 0, (strlength + 1));
  replace_sep(src, newSrcPath);

  userData.dest = NULL;
  userData.src = newSrcPath;
  userData.res = YES;

  if (-1 == fs_readdir(src, fs_remove_dir_callback, &userData)) {
    free(newSrcPath);
    return NO;
  }

  if (!userData.res) {
    free(newSrcPath);
    return NO;
  }

#ifdef _WIN32
  strlength = MultiByteToWideChar(CP_UTF8, 0, newSrcPath, -1, NULL, 0);
  wsrc = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wsrc == NULL) {
    free(newSrcPath);
    return NO;
  }
  memset(wsrc, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, newSrcPath, -1, wsrc, strlength);
  res = _wrmdir(wsrc);
  free(newSrcPath);
  free(wsrc);
  return res == 0;
#else
  res = rmdir(newSrcPath);
  free(newSrcPath);
  return res == 0;
#endif
}

boolean fs_mkdirs(string dir) {
#ifdef _WIN32
  wchar_t wdir[MAX_PATH];
#endif
  uint32_t i;
  string newPath; 
  uint32_t strlength;
  newPath = NULL;
  strlength = strlen(dir);
  newPath = (string)malloc((strlength + 2) * sizeof(char));
  if (newPath == NULL) {
    return NO;
  }
  memset(newPath, 0, (strlength + 2));
  replace_sep(dir, newPath);


#ifdef _WIN32
  memset(wdir, 0, MAX_PATH * sizeof(wchar_t));
  strlength = MultiByteToWideChar(CP_UTF8, 0, newPath, -1, NULL, 0);
  if (strlength < 2) {
    free(newPath);
    return NO;
  }
  MultiByteToWideChar(CP_UTF8, 0, newPath, -1, wdir, strlength);
  
  strlength--;
  if (L'\\' != wdir[strlength - 1]) {
    wcscat(wdir, L"\\");
    strlength++;
  }

  for (i = 1; i < strlength; i++) {
    if (L'\\' == wdir[i]) {
      wdir[i] = L'\0';
      if (wcscmp(wdir, L".") != 0 && !fs_is_dirw(wdir)) {
        if (_wmkdir(wdir) == -1) {
          if (fs_is_dir(newPath)) {
            free(newPath);
            return YES;
          } else {
            free(newPath);
            return NO;
          }
        }
      }
      wdir[i] = L'\\';
    }
  }

  free(newPath);
  return YES;
#else
  
  if ('/' != newPath[strlength - 1]) {
    strcat(newPath, "/");
    strlength++;
  }

  for (i = 1; i < strlength; i++) {
    if ('/' == newPath[i]) {
      newPath[i] = '\0';
      if (strcmp(newPath, ".") != 0 && !fs_is_dir(newPath)) {
        if (mkdir(newPath, 0777) == -1) {
          if (i != strlength - 1) newPath[i] = '/';
          if (fs_is_dir(newPath)) {
            free(newPath);
            return YES;
          } else {
            free(newPath);
            return NO;
          }
        }
      }
      newPath[i] = '/';
    }
  }

  free(newPath);
  return YES;
#endif
}

string replace_sep(string input, string output) {
  uint32_t i;
  uint32_t j;

  i = 0;
  j = 0;

  if (output == NULL || input == NULL) {
    return NULL;
  }

  while (output[j] != '\0') {
    output[j] = '\0';
    j++;
  }

  while (input[i] != '\0') {
#ifdef _WIN32
    if (input[i] == '/') {
      output[i] = '\\';
    } else {
      output[i] = input[i];
    }
#else
    if (input[i] == '\\') {
      output[i] = '/';
    } else {
      output[i] = input[i];
    }
#endif
    i++;
  }
  output[i] = '\0';
  return output;
}

int32_t fs_readdir(string path, ReaddirCallback callback, void* user_data) {
#ifdef _WIN32
  struct _wfinddata_t file;
  intptr_t hFile;
  char newPath[MAX_PATH];
  wchar_t wNewPath[MAX_PATH];
  int32_t strlength;
  int32_t written;
  char item[MAX_PATH];

  written = 0;

  memset(newPath, 0, MAX_PATH);
  replace_sep(path, newPath);
  strcat(newPath, "\\*.*");

  strlength = MultiByteToWideChar(CP_UTF8, 0, newPath, -1, NULL, 0);
  MultiByteToWideChar(CP_UTF8, 0, newPath, -1, wNewPath, strlength);

  hFile = _wfindfirst(wNewPath, &file);
  if (hFile == -1) {
    return -1;
  }

  strlength = WideCharToMultiByte(CP_UTF8, 0, file.name, -1, NULL, 0, NULL, NULL);
  memset(item, 0, MAX_PATH);
  WideCharToMultiByte(CP_UTF8, 0, file.name, -1, item, strlength, NULL, NULL);
  if (strcmp(item, ".") != 0 && strcmp(item, "..") != 0) {
    if (callback != NULL) {
      callback(item, (boolean)(file.attrib == _A_SUBDIR), (void*)user_data);
    }
    written++;
  }

  while (_wfindnext(hFile, &file) == 0) {
    strlength = WideCharToMultiByte(CP_UTF8, 0, file.name, -1, NULL, 0, NULL, NULL);
    memset(item, 0, MAX_PATH);
    WideCharToMultiByte(CP_UTF8, 0, file.name, -1, item, strlength, NULL, NULL);
    if (strcmp(item, ".") != 0 && strcmp(item, "..") != 0) {
      if (callback != NULL) {
        callback(item, (boolean)(file.attrib == _A_SUBDIR), (void*)user_data);
      }
      written++;
    }
  }
  _findclose(hFile);

  return written;
#else
  DIR* pDir;
  struct dirent* pEnt;
  uint32_t cnt;
  string newPath;
  uint32_t len;

  pDir = NULL;
  pEnt = NULL;
  cnt = 0;
  newPath = NULL;

  len = strlen(path);
  newPath = (string)malloc((len + 1) * sizeof(char));
  if (newPath == NULL) {
    return -1;
  }
  memset(newPath, 0, (len + 1));
  replace_sep(path, newPath);

  pDir = opendir(newPath);
  free(newPath);
  if (NULL == pDir) {
    return -1;
  }

  while ((pEnt = readdir(pDir)) != NULL) {
    if (strcmp(pEnt->d_name, ".") != 0 && strcmp(pEnt->d_name, "..") != 0) {
      if (callback != NULL) {
        callback(pEnt->d_name, (boolean)(pEnt->d_type == DT_DIR), (void*)user_data);
      }
      cnt++;
    }
  }

  closedir(pDir);
  return cnt;
#endif
}

#ifdef _WIN32
boolean run_executable(string argv) {
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  int32_t res;
  int32_t strlength;
  wchar_t* wargv;

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));

  strlength = MultiByteToWideChar(CP_UTF8, 0, argv, -1, NULL, 0);
  wargv = (wchar_t*)malloc(strlength * sizeof(wchar_t));
  if (wargv == NULL) {
    return NO;
  }
  memset(wargv, 0, strlength * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, argv, -1, wargv, strlength);

  res = CreateProcessW(
    NULL,
    wargv,
    NULL,
    NULL,
    FALSE,
    DETACHED_PROCESS,
    NULL,
    NULL,
    &si,
    &pi);
  free(wargv);

  if (res) {
    // WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return YES;
  }
  // DWORD dwErrCode = GetLastError();
  // printf("ErrCode : %d\n", dwErrCode);
  return NO;
}
#else
boolean run_executable(string file, string* argv) {
  pid_t pid = fork();
  switch(pid)
  {
    case -1:
      return NO;
    case 0:
      execvp(file, argv);
      return NO;
    default:
      return YES;
  }
}
#endif

boolean apply_patch(string tmpDir, string targetDir) {
  boolean res;
  res = fs_copy(tmpDir, targetDir);
  if (!res) {
	  return NO;
  }
  if (strcmp(tmpDir, targetDir) != 0) {
    return fs_remove(tmpDir);
  }
  return YES;
}

#ifdef _WIN32
int32_t start(int argc, wchar_t** argv) {
  string cmd, tmpDir, targetDir;
  int code, strlength;

  if (argc < 4) {
    return 3;
  }

  strlength = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);
  tmpDir = (string)malloc(strlength * sizeof(char));
  if (!tmpDir) {
    return 1;
  }
  memset(tmpDir, 0, strlength);
  WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, tmpDir, strlength, NULL, NULL);

  strlength = WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, NULL, 0, NULL, NULL);
  targetDir = (string)malloc(strlength * sizeof(char));
  if (!targetDir) {
    free(tmpDir);
    return 1;
  }
  memset(targetDir, 0, strlength);
  WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, targetDir, strlength, NULL, NULL);

  strlength = WideCharToMultiByte(CP_UTF8, 0, argv[3], -1, NULL, 0, NULL, NULL);
  cmd = (string)malloc(strlength * sizeof(char));
  if (!cmd) {
    free(tmpDir);
    free(targetDir);
    return 1;
  }
  memset(cmd, 0, strlength);
  WideCharToMultiByte(CP_UTF8, 0, argv[3], -1, cmd, strlength, NULL, NULL);

  if (apply_patch(tmpDir, targetDir)) {
    code = 0;
  } else {
    free(cmd);
    free(tmpDir);
    free(targetDir);
    return 2;
  }

  free(tmpDir);
  free(targetDir);

  if (run_executable(cmd)) {
    code = 0;
  } else {
    code = 4;
  }

  free(cmd);
  return code;
}
#else
int32_t start(int argc, char** argv) {
  string* cmd;
  int code, strlength, length, i;
  cmd = NULL;
  if (argc < 4) {
    return 3;
  }

  if (apply_patch(argv[1], argv[2])) {
    code = 0;
  } else {
    return 2;
  }

  length = argc - 4;

  if (length != 0) {
    cmd = (string*)malloc((length + 1) * sizeof(string));
    for (i = 0; i < length; i++) {
      cmd[i] = argv[i + 4];
    }
    cmd[i] = NULL;
  }

  if (run_executable(argv[3], cmd)) {
    code = 0;
  } else {
    code = 4;
  }
  free(cmd);
  return code;
}
#endif

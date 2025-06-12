// Minimal POSIX-ish surface for MSVC.  Mingw (and everything else) has the
// real headers; this shim only does anything when compiled with _MSC_VER.
//
// Verified against the UCRT shipped with VS2026: stat/fstat, struct stat,
// access, getcwd, mkdir, isatty, fileno, strdup and the S_IF* constants
// already exist; the symbols below are the only gaps the source tree uses.
#ifndef Z2UTIL_POSIX_SHIM_H
#define Z2UTIL_POSIX_SHIM_H

#if defined(_MSC_VER)

#include <direct.h>
#include <io.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef F_OK
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef S_ISREG
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISBLK(m)  0
#define S_ISLNK(m)  0
#define S_ISSOCK(m) 0
#endif

// UCRT defines S_IFMT/S_IFREG/S_IFDIR/S_IFCHR but not these two.
#ifndef S_IFIFO
#define S_IFIFO 0x1000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0x6000
#endif

#ifndef _MODE_T_DEFINED
typedef unsigned short mode_t;
#define _MODE_T_DEFINED
#endif

#ifndef S_IRWXU
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#endif

// libgen.h does not exist on MSVC.  The callers (util/file.cc) hand us a
// local writable buffer, so simple stateless copies suffice.
inline char *basename(char *path) {
    size_t n = strlen(path);
    if (n == 0) {
        return const_cast<char *>(".");
    }
    if (n == 1 && path[0] == '/') {
        return path;
    }
    char *end = path + n - 1;
    while (end > path && *end == '/') {
        *end-- = '\0';
    }
    char *slash = end;
    while (slash > path && *slash != '/' && *slash != '\\') {
        --slash;
    }
    return *slash == '/' || *slash == '\\' ? slash + 1 : path;
}

inline char *dirname(char *path) {
    size_t n = strlen(path);
    if (n == 0) {
        return const_cast<char *>(".");
    }
    char *end = path + n - 1;
    while (end > path && *end == '/') {
        *end-- = '\0';
    }
    char *slash = end;
    while (slash > path && *slash != '/' && *slash != '\\') {
        --slash;
    }
    if (slash == path) {
        return path;
    }
    *(slash + 1) = '\0';
    return path;
}

#endif  // _MSC_VER
#endif  // Z2UTIL_POSIX_SHIM_H
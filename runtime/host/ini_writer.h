// [XCDE] T247: shared flat-INI writer. Preserve keys owned by other/newer frontends.
#pragma once
#include <stdio.h>
#include <string.h>
#include "atomic_file.h"
#ifdef _WIN32
#include <process.h>
#define XCDE_INI_PID _getpid
#else
#include <unistd.h>
#define XCDE_INI_PID getpid
#endif

static inline int xcde_ini_key(const char *line, char *key, size_t cap) {
    const unsigned char *s = (const unsigned char *)line;
    if (strlen(line) >= 3 && s[0] == 239 && s[1] == 187 && s[2] == 191) s += 3;
    while (*s == ' ' || *s == '\t') ++s;
    if (!*s || *s == '#' || *s == ';' || *s == '[') return 0;
    const char *eq = strchr((const char *)s, '=');
    if (!eq) return 0;
    const char *end = eq;
    while (end > (const char *)s && (end[-1] == ' ' || end[-1] == '\t')) --end;
    size_t n = (size_t)(end - (const char *)s);
    if (!n || n >= cap) return 0;
    memcpy(key, s, n); key[n] = 0;
    return 1;
}

static inline FILE *xcde_ini_begin(const char *path, char *tmp, size_t cap) {
    static unsigned serial;
    int n = snprintf(tmp, cap, "%s.%lu.%u.tmp", path, (unsigned long)XCDE_INI_PID(), ++serial);
    if (n < 0 || (size_t)n >= cap) return NULL;
    return fopen(tmp, "w+");
}

static inline int xcde_ini_commit(FILE *out, const char *tmp, const char *path) {
    int ok = !ferror(out) && fflush(out) == 0;
    long generated_end = ok ? ftell(out) : -1;
    if (generated_end < 0) ok = 0;
    FILE *old = ok ? fopen(path, "r") : NULL;
    char line[4096], key[512], fresh[4096], fresh_key[512];
    while (old && ok && fgets(line, sizeof line, old)) {
        // Refuse overlong input rather than silently truncate a future setting.
        if (!strchr(line, '\n') && !feof(old)) { ok = 0; break; }
        if (!xcde_ini_key(line, key, sizeof key)) continue;
        rewind(out);
        int found = 0;
        while (ftell(out) < generated_end && fgets(fresh, sizeof fresh, out)) {
            if (xcde_ini_key(fresh, fresh_key, sizeof fresh_key) && !strcmp(key, fresh_key)) { found = 1; break; }
        }
        if (ferror(out) || fseek(out, 0, SEEK_END) != 0) { ok = 0; break; }
        if (!found && (fputs(line, out) == EOF || (!strchr(line, '\n') && fputc('\n', out) == EOF))) ok = 0;
    }
    if (old) { if (ferror(old)) ok = 0; if (fclose(old) != 0) ok = 0; }
    if (fclose(out) != 0) ok = 0;
    if (ok) ok = xcde_atomic_replace(tmp, path);
    if (!ok) remove(tmp);
    return ok;
}

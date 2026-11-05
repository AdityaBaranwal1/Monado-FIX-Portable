// [XCDE] T260: bounded, four-way descriptor cache; mutex covers membership and active-user references.
#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "../render_policy.h"
#define XCDE_FD_CACHE 256
typedef struct { int file,fd,users; uint64_t stamp; } XcdeFdEntry;
static inline int xcde_fd_acquire(XcdeFdEntry *entries,pthread_mutex_t *lock,uint64_t *stamp,
                                 int idx,const char *path,int flags,int *slot) {
    int base=(idx%(XCDE_FD_CACHE/4))*4, files[4],users[4],fd; uint64_t ages[4];
    pthread_mutex_lock(lock);
    for (int i=0;i<4;i++) { files[i]=entries[base+i].file; users[i]=entries[base+i].users; ages[i]=entries[base+i].stamp; }
    int way=xcde_fd_slot(files,users,ages,4,idx), s=way>=0 ? base+way : -1;
    if (s>=0) entries[s].stamp=++*stamp;
    if (s>=0 && entries[s].file==idx && entries[s].fd>=0) {
        entries[s].users++; *slot=s; fd=entries[s].fd;
    } else if (s>=0 && !entries[s].users) {
        if (entries[s].fd>=0) close(entries[s].fd);
        entries[s].fd=open(path,flags); entries[s].file=idx;
        entries[s].users=entries[s].fd>=0;
        *slot=entries[s].fd>=0 ? s : -1; fd=entries[s].fd;
    } else { *slot=-1; fd=open(path,flags); }
    pthread_mutex_unlock(lock); return fd;
}
static inline void xcde_fd_release(XcdeFdEntry *entries,pthread_mutex_t *lock,int fd,int slot) {
    if (slot>=0) { pthread_mutex_lock(lock); entries[slot].users--; pthread_mutex_unlock(lock); }
    else if (fd>=0) close(fd);
}

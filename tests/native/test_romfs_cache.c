// [XCDE] T260: real file_fd users/refcounts under colliding concurrent readers, no game data.
#include <assert.h>
#include <io.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../runtime/platform.h"
#include "../../runtime/services/romfs_fd_cache.h"
#define FD_CACHE XCDE_FD_CACHE
typedef struct { const char *path; } File;
static File **g_files;
static XcdeFdEntry g_fds[FD_CACHE];
static pthread_mutex_t g_fd_lock=PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_stamp;
static int file_fd(int idx,int *slot) { return xcde_fd_acquire(g_fds,&g_fd_lock,&g_stamp,idx,g_files[idx]->path,O_RDONLY|O_BINARY,slot); }
static void file_fd_release(int fd,int slot) { xcde_fd_release(g_fds,&g_fd_lock,fd,slot); }
static int indices[]={0,256,512,768,1024};
static void verify(int fd,int i) {
    unsigned char byte=0; DWORD got=0; OVERLAPPED offset={0};
    assert(ReadFile((HANDLE)_get_osfhandle(fd),&byte,1,&got,&offset) && got==1 && byte==(unsigned char)(i+1));
}
static void *reader(void *arg) {
    int i=(int)(intptr_t)arg;
    for (int n=0;n<1000;n++) { int slot,fd=file_fd(indices[i],&slot); assert(fd>=0); verify(fd,i); file_fd_release(fd,slot); }
    return NULL;
}
int main(int argc,char **argv) {
    assert(argc==2); g_files=calloc(1025,sizeof *g_files); assert(g_files);
    File entries[5]={0}; char paths[5][1024]; int fd[5],slot[5];
    for (int i=0;i<FD_CACHE;i++) g_fds[i].file=g_fds[i].fd=-1;
    for (int i=0;i<5;i++) {
        snprintf(paths[i],sizeof paths[i],"%s/cache-%d.bin",argv[1],i);
        FILE *out=fopen(paths[i],"wb"); assert(out); fputc(i+1,out); fclose(out);
        entries[i].path=paths[i]; g_files[indices[i]]=&entries[i];
        fd[i]=file_fd(indices[i],&slot[i]); assert(fd[i]>=0); verify(fd[i],i);
    }
    assert(slot[4]==-1); // all four ways are pinned; fifth reader must own a private descriptor
    file_fd_release(fd[0],slot[0]);
    int new_slot,new_fd=file_fd(indices[4],&new_slot); assert(new_fd>=0 && new_slot>=0); verify(new_fd,4);
    for (int i=1;i<5;i++) { verify(fd[i],i); file_fd_release(fd[i],slot[i]); }
    file_fd_release(new_fd,new_slot);
    pthread_t threads[5];
    for (int i=0;i<5;i++) assert(!pthread_create(&threads[i],NULL,reader,(void *)(intptr_t)i));
    for (int i=0;i<5;i++) assert(!pthread_join(threads[i],NULL));
    for (int i=0;i<FD_CACHE;i++) { assert(!g_fds[i].users); if (g_fds[i].fd>=0) close(g_fds[i].fd); }
    for (int i=0;i<5;i++) assert(!remove(paths[i]));
    free(g_files); puts("romfs descriptor cache: saturated ways, private fallback and concurrent read ownership passed"); return 0;
}

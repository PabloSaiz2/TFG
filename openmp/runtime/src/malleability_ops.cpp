#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "kmp.h"
#include "schedctl.h"
#include <stdio.h>
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define SCHEDCTL_PROC_ENTRY "/proc/pmc/schedctl"

schedctl_t* master_thread_schedctl=NULL;

static int test_schedctl_support() {
    /* Just make sure the proc entry exists. If it does, close it.*/
    int fd = open(SCHEDCTL_PROC_ENTRY, O_RDWR);
    if(fd < 0) {
        return 0;
    }
    else {
        close(fd);
        return 1;
    }
}

void schedctl_release(struct kmp_info_t* thr) {
    if (thr->th.schedctl_data)
        munmap(thr->th.schedctl_data, PAGE_SIZE);
    if (thr->th.schedctl_fd!=-1)
        close(thr->th.schedctl_fd);
    else
        free(thr->th.schedctl_data); //emulated version
    /* Reset fields */
    thr->th.schedctl_data=NULL;
    thr->th.schedctl_fd=-1;
}

schedctl_t* schedctl_retrieve(struct kmp_info_t* thr) {
    static int first_time=1;
    static int schedctl_support=0;
    int configfd;
    schedctl_t *schedctl = NULL;
    /* Reset fields ... */
    thr->th.schedctl_fd=-1;
    thr->th.schedctl_data=NULL;
    if (first_time) {
        schedctl_support=test_schedctl_support();
        first_time=0;
    }
    if (schedctl_support) {
        configfd = open(SCHEDCTL_PROC_ENTRY, O_RDWR);
        if(configfd < 0) {
            perror("open");
            return NULL;
        }
        schedctl=(schedctl_t*)mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED, configfd, 0);
        if (schedctl == MAP_FAILED) {
            perror("mmap");
            return NULL;
        }
        /*
        if (gomp_debug_var>=3) {
            printf("Mmap Ok. Address:0x%p\n",schedctl);
            printf("%d %d\n", schedctl->sc_nfc,schedctl->sc_sf);
        }
        */
        if(gomp_malleable)
            schedctl->sc_malleable=1;

        schedctl->sc_num_threads= __kmp_initial_threads;
    }
    else{

    }
    thr->th.schedctl_data=schedctl;
    thr->th.schedctl_fd=configfd;
    return schedctl;
}

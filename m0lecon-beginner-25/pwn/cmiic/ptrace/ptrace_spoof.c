#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <sys/ptrace.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// Puntatori a funzioni reali
static int (*real_open)(const char *pathname, int flags, ...) = NULL;
static FILE *(*real_fopen)(const char *pathname, const char *mode) = NULL;
static long (*real_ptrace)(enum __ptrace_request request, ...) = NULL;

// Constructor: stampa un messaggio quando la libreria viene caricata
__attribute__((constructor))
void init_library() {
    fprintf(stderr, "[ptrace.so] Libreria caricata con successo (LD_PRELOAD attivo)\n");
}

// --- Spoof ptrace(PTRACE_TRACEME)
long ptrace(enum __ptrace_request request, ...) {
    if (!real_ptrace)
        real_ptrace = dlsym(RTLD_NEXT, "ptrace");

    if (request == PTRACE_TRACEME) {
        // Spoof: finge che non ci sia tracciamento
        return 0;
    }

    va_list args;
    va_start(args, request);
    void *pid = va_arg(args, void *);
    void *addr = va_arg(args, void *);
    void *data = va_arg(args, void *);
    long ret = real_ptrace(request, pid, addr, data);
    va_end(args);
    return ret;
}

// Intercetta fopen
FILE *fopen(const char *pathname, const char *mode) {
    if (!real_fopen)
        real_fopen = dlsym(RTLD_NEXT, "fopen");

    if (strstr(pathname, "/proc/self/status")) {
        return real_fopen("/tmp/fake_status", mode);
    }

    return real_fopen(pathname, mode);
}

// Intercetta open
int open(const char *pathname, int flags, ...) {
    if (!real_open)
        real_open = dlsym(RTLD_NEXT, "open");

    if (strstr(pathname, "/proc/self/status")) {
        return real_open("/tmp/fake_status", flags);
    }

    va_list args;
    va_start(args, flags);
    int fd = real_open(pathname, flags, va_arg(args, int));
    va_end(args);
    return fd;
}

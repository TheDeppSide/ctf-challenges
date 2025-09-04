# catch me if you can

## Category
pwn + rev

## Build
Compila su Linux (serve `libseccomp-dev`):

```bash
gcc chall.c -o cmiiy -no-pie -z execstack -fno-stack-protector -lseccomp -lm -s
```

Check con `checksec ./cmiiy`:  
- Canary: Enabled  
- NX: Disabled  
- PIE: No PIE  
- RELRO: None  

## Run
```bash
./cmiiy
# oppure
socat TCP-LISTEN:31337,reuseaddr,fork EXEC:./cmiiy
```

## Mitigations
- ✅ **Canary ON** (vero `__stack_chk_guard`, leakato dopo il gate)  
- ❌ **NX OFF** (stack eseguibile)  
- ❌ **PIE OFF** (indirizzi fissi, ret2win facile)  
- ✅ **Seccomp sandbox** (blocca `execve`, `fork`, `clone`, `ptrace` ecc.)

## Rev phase (Gate)
- Stampa un **orario target random**.  
- Input: **lat lon** (lat accetta anche `size_hint`).  
- Check =  
  - `encode_time16()` su orario & input  
  - rotazione/traslazione coordinate → `rx,ry`  
  - `mac16()` CRC su `(rx,ry,seed,enc)`  
- Serve reverse-engineering di encoding e MAC per trovare `(lat,lon)` validi.  
- Se corretto → leak del **canary**.

## Pwn phase
- **Overflow #1 (stack):**
  ```c
  char name[64];
  gets(name); // Canary ON
  ```
- **Overflow #2 (heap):**
  ```c
  char *msg = malloc(user_size);
  read(0, msg, user_size*4); // oversize read
  ```
- La `lat` può includere un **secondo numero** = `user_size`.  
- Exploit path classico: usare il leak del canary e fare **ret2win** (indirizzi fissi, NX off, ma seccomp blocca `exec*`).

## Player instructions
- Inserisci `lat lon` → trova valori validi per passare il gate.  
- Output mostra:
  ```
  ... his name is 0x<canary>
  ```
- Usa il canary nell’overflow per non crashare e saltare a `win()`.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
// requires: sudo apt-get install libseccomp-dev
#include <seccomp.h>
#include <sys/prctl.h>
#include <sys/resource.h>

// extern uintptr_t __stack_chk_guard; // vero canary del compilatore

// ===================== Config ===================== //
#define TOL_MIN   4
#define TIMEOUT_S 60
#define FLAG_SOURCE "flag.txt"

// ===================== Utils ====================== //
void helping_lazy_bss_writer(char *s){
    puts(FLAG_SOURCE);
}
__attribute__((naked)) void pop_rdi_ret() {
    __asm__("pop %rdi; ret;");
}
int really_sus_func(const char *filename) {
    FILE *f= fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return -1;
    }

    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (fwrite(buf, 1, n, stdout) != n) {
            perror("fwrite");
            fclose(f);
            return -1;
        }
    }

    if (ferror(f)) {
        perror("fread");
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

static void set_unbuffered(void){
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
    setvbuf(stdin ,NULL,_IONBF,0);
}

static void read_line(char *buf, size_t n){
    size_t i=0; int c;
    while(i+1<n && (c=fgetc(stdin))!=EOF && c!='\n') buf[i++]=(char)c;
    buf[i]='\0';

    // Easter egg
    int cnt=0;
    if (n>20) {
        for (size_t j=0; j<n; j++){
            if (buf[j] == 'A' || buf[j] == 'a'){
                cnt++;
            }
        }
        if(cnt>20){
            puts("\n[BATHROOM] So you're one of those who run random binaries full of A's without knowing what's inside...");
            puts("This time you got lucky. Next time, maybe not.");
            _exit(0);
        }
    }
}

static int minutes_mod(int m){
    m%=1440; if(m<0) m+=1440; return m;
}

// CRC32 (0xEDB88320)
static uint32_t crc32_bytes(const uint8_t *data, size_t len){
    uint32_t crc = 0xFFFFFFFFu;
    for(size_t i=0;i<len;i++){
        crc ^= (uint32_t)data[i];
        for(int b=0;b<8;b++){
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

static void u32le(uint8_t *p, uint32_t v){ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static void u16le(uint8_t *p, uint16_t v){ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }

static void derive_params(unsigned seed, double *theta, double *bx, double *by, uint32_t *k1, uint32_t *k2){
    *theta = ((double)((seed * 2654435761u) & 0xFFFF) / 65535.0) * (2.0*M_PI);
    *bx = ((int)((seed >> 16) & 0xFF) - 128) / 10.0;
    *by = ((int)((seed >> 24) & 0xFF) - 128) / 10.0;
    *k1 = 0xA5F0C3B1u ^ seed;
    *k2 = 0x9E3779B1u ^ (seed<<1);
}

static void rotate_coords(double lat, double lon, double theta, double bx, double by, double *ox, double *oy){
    double c = cos(theta), s = sin(theta);
    *ox = c*lat - s*lon + bx;
    *oy = s*lat + c*lon + by;
}

static uint16_t encode_time16(int minutes, uint32_t k1){
    int m = minutes; if(m < 0) m += ((-m/1440)+1)*1440; m %= 1440;
    return (uint16_t)(((uint32_t)(m * 1337u) ^ k1) & 0xFFFFu);
}

static uint16_t mac16(double rx, double ry, uint32_t seed, uint32_t k2, uint16_t enc){
    int32_t xi = (int32_t)llround(rx * 1000000.0);
    int32_t yi = (int32_t)llround(ry * 1000000.0);

    uint8_t buf[4+4+4+2];
    u32le(buf+0,  (uint32_t)xi);
    u32le(buf+4,  (uint32_t)yi);
    u32le(buf+8,  seed);
    u16le(buf+12, enc);

    uint32_t c = crc32_bytes(buf, sizeof buf);
    return (uint16_t)((c ^ k2) & 0xFFFFu);
}

// ===================== Security ====================== //
static void anti_debug_soft(void){
    struct rlimit rl = {0,0};
    setrlimit(RLIMIT_CORE, &rl);
    prctl(PR_SET_DUMPABLE, 0);
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
}

static void block_shell_and_debug(void){
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
    if(!ctx) _exit(1);

    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve),    0);
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execveat),  0);
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(fork),      0);
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(vfork),     0);
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(clone),     0);

    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(ptrace),           0);
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(process_vm_readv), 0);
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(process_vm_writev),0);

    if (seccomp_load(ctx) != 0) _exit(1);
    seccomp_release(ctx);
}

static void enable_sandbox(void){
    anti_debug_soft();
    block_shell_and_debug();
}

static int is_traced(void){
    FILE *f = fopen("/proc/self/status","r");
    if(!f) return 0;
    int tracer=0; char line[256];
    while(fgets(line,sizeof line,f)){
        if(!strncmp(line,"TracerPid:",10)){ sscanf(line+10,"%d",&tracer); break; }
    }
    fclose(f);
    return tracer>0;
}

// ===================== Banner ===================== //
static void show_banner(int target_local_minutes){
    puts(
"   ______      __            __        __  __       ___           __              \n"
"  / ____/___  / /____  _____/ /_____ _/ /_/ /_     /   |  ____ _ / /_____  ____ _ \n"
" / /   / __ \\/ __/ _ \\/ ___/ __/ __ `/ __/ __ \\   / /| | / __ `// //_/ _ \\/ __ `/ \n"
"/ /___/ /_/ / /_/  __(__  ) /_/ /_/ / /_/ / / /  / ___ |/ /_/ // ,< /  __/ /_/ /  \n"
"\\____/\\____/\\__/\\___/____/\\__/\\__,_/\\__/_/ /_/  /_/  |_|\\__,_//_/|_|\\___/\\__,_/   \n"
"                                 catch me if you can                               \n"
"-----------------------------------------------------------------------------------\n"
"[SOMEWHERE] Hi, I'm Frank Abagnale. I used to forge money; now I steal flags for fun.\n"
"If you want one, you'll have to catch me first. Oops-it's already "
    );
    printf("%02d:%02d", target_local_minutes/60, target_local_minutes%60);
    puts(" ... gotta run!\n");
}

// ===================== Stage 2 (pwn) =============== //
void win(void){
    set_unbuffered();
    system("/bin/sh");
    _exit(0);
}

static double g_lat, g_lon;
static size_t g_dynsz;
static unsigned long long g_canary;

// FIXA QUA, USA IL CONVERSION MISMATCH
static void pwn_room(double lat, double lon, size_t user_size){
    puts("\n[PTM'S SERVER ROOM] Well well... you actually *caught* me. Frankly impressive.");
    puts("Since you're here, prove it: sign my totally-safe guestbook.");

    char name[64];
    puts("Name, for the record (and the court):");
    // VULN 1: stack overflow (intentional)
    gets(name);
    

    printf("Nice alias, %s. Very undercover.\n", name);

    // VULN 2: oversized read (intentional)
    size_t sz = user_size ? user_size : 128;
    if (sz > (1u<<20)) sz = (1u<<20);
    char *msg = (char*)malloc(sz);
    if(!msg){ puts("Memory? Forged. Not today."); return; }

    puts("Now leave a *long* motivational note for future impostors:");
    read(0, msg, sz * 4);

    puts("Bold move. If you can overflow this, you can overflow anything.");
    free(msg);
}


// ===================== Stage 1 (gate) ============== //
static int solar_gate(void){
    time_t now = time(NULL);

    // Use UTC as the reference clock
    struct tm g; gmtime_r(&now, &g);
    int utc_minutes = g.tm_hour*60 + g.tm_min;

    // Random target time every run
    unsigned seed = (unsigned)(now ^ (now>>32) ^ (unsigned)getpid());
    srand(seed);
    int target_local_minutes = rand() % 1440;

    show_banner(target_local_minutes);

    double lat=0.0, lon=0.0;
    char buf[256];

    puts("[SOMEWHERE ELSE] Enter coordinates (solar time, not political timezones).");
    printf("Latitude  [-90..90] : ");
    read_line(buf, sizeof(buf));

    // conversion mismatch
    long long size_hint = 0;
    if (sscanf(buf, "%lf %lld", &lat, &size_hint) < 1){
        puts("\nInvalid latitude. skill issue");
        return 0;
    }
    if (size_hint <= 0) size_hint = 128;
    if ((unsigned long long)size_hint > (1ull<<20))
        size_hint = (1<<20);

    printf("Longitude [-180..180]: ");
    read_line(buf, sizeof(buf));
    if (sscanf(buf, "%lf", &lon) != 1){
        puts("\nInvalid longitude. skill issue");
        return 0;
    }

    if(lat<-90.0 || lat>90.0 || lon<-180.0 || lon>180.0){
        puts("\nCoordinates out of range. bro that's not Earth");
        return 0;
    }

    // Solar time ≈ UTC + lon * 4 minutes/°
    int local_from_lon = minutes_mod(utc_minutes + (int)lround(lon * 4.0));
    double theta, bx, by, rx, ry;
    uint32_t k1, k2;
    derive_params(seed, &theta, &bx, &by, &k1, &k2);
    rotate_coords(lat, lon, theta, bx, by, &rx, &ry);

    uint16_t enc_target = encode_time16(target_local_minutes, k1);
    uint16_t enc_input  = encode_time16(local_from_lon,      k1);

    uint16_t mac_input  = mac16(rx, ry, seed, k2, enc_input);
    uint16_t mac_expect = mac16(rx, ry, seed, k2, enc_target);

    if (enc_input == enc_target && mac_input == mac_expect){
        g_lat = lat; g_lon = lon; g_dynsz = (size_t)size_hint;

        printf(
            "\n[SOMEWHERE NEAR YOU] You're so close you might see my bird flying around — his name is 0x%llx.\n"
            "I know it sounds weird, but my wife picked it; sometimes she forgets and calls him other names.\n",
            (unsigned long long)g_canary
        );
        return 1;
    } else {
        puts("\n[SOMEWHERE TOO FAR] Nope. catch me if you *can't*");
        return 0;
    }
}

// ========================= main ==================== //
int main(void){
    alarm(TIMEOUT_S);
    set_unbuffered();
    enable_sandbox();

    if (is_traced()){
        puts("Nope. Catch me without a leash (detach your debugger)");
        _exit(0);
    }

    // Flavor-only canary value, considering to switch to the real canary
    // g_canary = (unsigned long long)(uintptr_t)__stack_chk_guard;
    g_canary = (unsigned long long)(uintptr_t)0x9E3779B1u;

    if(!solar_gate()){
        puts("\nCoordinates go brrr");
        return 0;
    }

    pwn_room(g_lat, g_lon, g_dynsz);
    return 0;
}

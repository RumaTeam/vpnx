// Small libc gaps between newlib (devkitA64/libnx) and what the ported libraries expect.
// Shared by the test apps and, later, the vpinball port.
#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>

// bimg's ASTC decoder uses posix_memalign; newlib has memalign but not this.
int posix_memalign(void** out, size_t alignment, size_t size)
{
    // POSIX: alignment must be a power of two and a multiple of sizeof(void*)
    if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0)
        return EINVAL;
    void* p = memalign(alignment, size);
    if (!p)
        return ENOMEM;
    *out = p;
    return 0;
}

// implot uses timegm (GNU/BSD extension, absent from newlib). Inverse of gmtime: tm is UTC.
#include <time.h>
time_t timegm(struct tm* tm)
{
    static const int cumdays[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    long year = tm->tm_year + 1900L + tm->tm_mon / 12;
    int mon = tm->tm_mon % 12;
    if (mon < 0) { mon += 12; year--; }
    long days = (year - 1970) * 365 + ((year - 1969) / 4 - (year - 1901) / 100 + (year - 1601) / 400)
              + cumdays[mon] + tm->tm_mday - 1;
    const int leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    if (leap && mon > 1) days++;
    return (((time_t)days * 24 + tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec;
}

// imgui's default OpenInShell uses fork/exec; there are no processes on Horizon. Stubs make it a no-op failure.
#include <sys/types.h>
pid_t waitpid(pid_t pid, int* status, int options) { (void)pid; (void)status; (void)options; errno = ECHILD; return -1; }
int execvp(const char* file, char* const argv[]) { (void)file; (void)argv; errno = ENOSYS; return -1; }

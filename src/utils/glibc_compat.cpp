#include <cstdarg>
#include <cstddef>

// GCC 15 on current WSL headers redirects the traditional conversion APIs to
// GLIBC 2.38 C23 symbols. Dragonwilds currently runs on GLIBC 2.36. Provide the
// equivalent pre-C23 entry points locally so the mod remains loadable there.
extern "C" long old_strtol(const char*, char**, int) asm("strtol");
extern "C" unsigned long old_strtoul(const char*, char**, int) asm("strtoul");
extern "C" long long old_strtoll(const char*, char**, int) asm("strtoll");
extern "C" unsigned long long old_strtoull(const char*, char**, int) asm("strtoull");
extern "C" int old_vsscanf(const char*, const char*, va_list) asm("__isoc99_vsscanf");

extern "C" long __isoc23_strtol(const char* value, char** end, int base) {
    return old_strtol(value, end, base);
}
extern "C" unsigned long __isoc23_strtoul(const char* value, char** end, int base) {
    return old_strtoul(value, end, base);
}
extern "C" long long __isoc23_strtoll(const char* value, char** end, int base) {
    return old_strtoll(value, end, base);
}
extern "C" unsigned long long __isoc23_strtoull(const char* value, char** end, int base) {
    return old_strtoull(value, end, base);
}
extern "C" int __isoc23_sscanf(const char* value, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int result = old_vsscanf(value, format, arguments);
    va_end(arguments);
    return result;
}

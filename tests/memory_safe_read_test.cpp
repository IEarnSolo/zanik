#include "utils/memory.h"

#include <cassert>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    std::uint64_t source = 0x0123456789abcdefULL;
    std::uint64_t copied = 0;
    assert(Mem::SafeRead(reinterpret_cast<std::uintptr_t>(&source), copied));
    assert(copied == source);

    const long pageSize = sysconf(_SC_PAGESIZE);
    assert(pageSize > 0);
    void* page = mmap(nullptr, static_cast<std::size_t>(pageSize), PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(page != MAP_FAILED);
    *reinterpret_cast<std::uint64_t*>(page) = source;
    assert(Mem::SafeRead(reinterpret_cast<std::uintptr_t>(page), copied));
    assert(munmap(page, static_cast<std::size_t>(pageSize)) == 0);

    // This is the NPC crash condition: a pointer passed validation and was
    // unmapped before it could be copied. SafeRead must fail closed instead
    // of dereferencing it in userspace and terminating the server.
    copied = 0;
    assert(!Mem::SafeRead(reinterpret_cast<std::uintptr_t>(page), copied));
    assert(!Mem::SafeRead(0, copied));
    return 0;
}


#include <stdlib.h>
#include <sys/mman.h>

#include "spew.hpp"
#include "memalloc.hpp"

namespace Whisper {


void *
AllocateMemory(size_t bytes)
{
    void *ptr = malloc(bytes);
    SpewMemoryNote("AllocateMemory allocated %ld bytes at %p",
                   (long)bytes, ptr);
    return ptr;
}

void
ReleaseMemory(void *ptr)
{
    SpewMemoryNote("ReleasingMemory releasing %p", ptr);
    free(ptr);
}


void *
AllocateMappedMemory(size_t bytes, bool allowExec)
{
    int prot = PROT_READ | PROT_WRITE;
    if (allowExec)
        prot |= PROT_EXEC;

    void *result = mmap(nullptr, bytes, prot,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, 0);
    if (result == MAP_FAILED) {
        SpewMemoryError("AllocateMappedMemory failed to map %ld bytes",
                        (long)bytes);
        return nullptr;
    }

    SpewMemoryNote("AllocateMappedMemory mapped %ld bytes at %p (exec=%s)",
                   (long)bytes, result, allowExec?"yes":"no");

    return result;
}

bool
ReleaseMappedMemory(void *ptr, size_t bytes)
{
    SpewMemoryNote("ReleaseMappedMemory unmapping %ld bytes at %p",
                   (long)bytes, ptr);
    int result = munmap(ptr, bytes) == 0;
    if (result == -1) {
        SpewMemoryError("ReleaseMappedMemory failed to unmap %p.",
                        (long)bytes, ptr);
        return false;
    }
    return true;
}


} // namespace Whisper


#include <stdlib.h>
#include <sys/mman.h>

#include "memalloc.hpp"

namespace Whisper {


void *
AllocateMemory(size_t bytes)
{
    return malloc(bytes);
}

void
Release(void *ptr)
{
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
    if (result == MAP_FAILED)
        return nullptr;

    return result;
}

bool
ReleaseAligned(void *ptr, size_t bytes)
{
    return munmap(ptr, bytes) == 0;
}


} // namespace Whisper

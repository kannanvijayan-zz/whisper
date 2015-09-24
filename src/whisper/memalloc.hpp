#ifndef WHISPER__MEMALLOC_HPP
#define WHISPER__MEMALLOC_HPP

#include "common.hpp"

//
// Low-level allocation of memory.
//

namespace Whisper {


// Allocate a certain number of bytes.
//
// Returns NULL on failure.
void* AllocateMemory(size_t bytes);
void ReleaseMemory(void* ptr);


// Allocates an amount of mmap-ed memory.
//
// Returns NULL on failure.
void* AllocateMappedMemory(size_t bytes, bool allowExec=false);
bool ReleaseMappedMemory(void* ptr, size_t bytes);



} // namespace Whisper

#endif // WHISPER__MEMALLOC_HPP

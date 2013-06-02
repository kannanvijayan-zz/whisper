#ifndef WHISPER__COMMON_HPP
#define WHISPER__COMMON_HPP

#include <inttypes.h>
#include "config.h"

// Exactly one of ARCH_BITS_32 or ARCH_BITS_64 should be defined.
#if defined(ARCH_BITS_32) && defined(ARCH_BITS_64)
#   error "Both ARCH_BITS_32 and ARCH_BITS_64 are defined!"
#endif

#if !defined(ARCH_BITS_32) && !defined(ARCH_BITS_64)
#   error "Neither ARCH_BITS_32 nor ARCH_BITS_64 are defined!"
#endif

namespace Whisper {


#if defined(ARCH_BITS_32)
constexpr unsigned WordBits = 32;
constexpr unsigned WordBytes = 4;
typedef uint32_t word_t;
typedef int32_t sword_t;
    
#else // defined(ARCH_BITS_64)
constexpr unsigned WordBits = 64;
constexpr unsigned WordBytes = 8;
typedef uint64_t word_t;
typedef int64_t sword_t;

#endif


// Casting between pointers and words.
template <typename T>
constexpr inline word_t PtrToWord(T *ptr) {
    return static_cast<word_t>(ptr);
}
template <typename T>
constexpr inline T *WordToPtr(word_t word) {
    return static_cast<word_t>(word);
}


} // namespace Whisper

#endif // WHISPER__COMMON_HPP

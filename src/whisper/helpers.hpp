#ifndef WHISPER__HELPERS_HPP
#define WHISPER__HELPERS_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {

// Check if an integer type is a power of two.
template <typename IntT>
inline bool IsPowerOfTwo(IntT value) {
    return (value & (value - 1)) == 0;
}

// Align integer types
template <typename IntT>
inline IntT AlignIntDown(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return value - (value & (align -1));
}

template <typename IntT>
inline IntT AlignIntUp(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return value + (align - (value & (align -1)));
}

// Align pointer types
template <typename PtrT, typename IntT>
inline PtrT *AlignPtrDown(PtrT *ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return WordToPtr<PtrT>(AlignIntDown<word_t>(PtrToWord(ptr), align));
}

template <typename PtrT, typename IntT>
inline PtrT *AlignPtrUp(PtrT *ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return WordToPtr<PtrT>(AlignIntUp<word_t>(PtrToWord(ptr), align));
}

// Max of two integers.
template <typename IntT>
inline IntT Max(IntT a, IntT b) {
    return (a >= b) ? a : b;
}

template <typename IntT>
inline IntT Min(IntT a, IntT b) {
    return (a <= b) ? a : b;
}


} // namespace Whisper

#endif // WHISPER__HELPERS_HPP

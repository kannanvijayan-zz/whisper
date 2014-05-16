#ifndef WHISPER__VM__STRING_SPECIALIZATIONS_HPP
#define WHISPER__VM__STRING_SPECIALIZATIONS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
#include "gc.hpp"
#include <cstring>

#include <new>

//
// GC-Specializations for String
//
namespace Whisper {
namespace GC {


template <>
struct HeapTraits<VM::String>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format = GC::AllocFormat::UntracedThing;

    // All constructor signatures for Array take the length as the
    // first argument.
    template <typename... Args>
    static uint32_t SizeOf(uint32_t len, Args... rest) {
        return sizeof(VM::String) + len;
    }

    static uint32_t SizeOf(const char *str) {
        return sizeof(VM::String) + strlen(str);
    }

    static uint32_t SizeOf(const VM::String &other) {
        return AllocThing::From(&other)->size();
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__STRING_SPECIALIZATIONS_HPP

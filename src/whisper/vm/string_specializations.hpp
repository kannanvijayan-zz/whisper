#ifndef WHISPER__VM__STRING_SPECIALIZATIONS_HPP
#define WHISPER__VM__STRING_SPECIALIZATIONS_HPP

#include "vm/core.hpp"

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
    static constexpr bool VarSized = true;
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__STRING_SPECIALIZATIONS_HPP

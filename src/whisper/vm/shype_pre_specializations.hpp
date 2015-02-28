#ifndef WHISPER__VM__SHYPE_PRE_SPECIALIZATIONS_HPP
#define WHISPER__VM__SHYPE_PRE_SPECIALIZATIONS_HPP

#include "vm/core.hpp"

#include <cstring>
#include <new>

//
// GC-Specializations for String
//
namespace Whisper {
namespace GC {


template <>
struct AllocThingTraits<VM::Shype>
{
    AllocThingTraits() = delete;
    static constexpr bool Specialized = true;
};

template <>
struct HeapTraits<VM::RootShype>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format = GC::AllocFormat::RootShype;
    static constexpr bool VarSized = false;
};

template <>
struct HeapTraits<VM::AddSlotShype>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format = GC::AllocFormat::AddSlotShype;
    static constexpr bool VarSized = false;
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__SHYPE_PRE_SPECIALIZATIONS_HPP

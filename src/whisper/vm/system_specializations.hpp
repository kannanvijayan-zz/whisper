#ifndef WHISPER__VM__SYSTEM_SPECIALIZATIONS_HPP
#define WHISPER__VM__SYSTEM_SPECIALIZATIONS_HPP


#include "vm/core.hpp"
#include "vm/module.hpp"

#include <new>

namespace Whisper {
namespace GC {

template <>
struct TraceTraits<VM::System>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::System &system,
                     const void *start, const void *end)
    {
        system.modules_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::System &system,
                       const void *start, const void *end)
    {
        system.modules_.update(updater, start, end);
    }
};


template <>
struct HeapTraits<VM::System>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::System;

    template <typename... Args>
    static uint32_t SizeOf(Args... rest) {
        return sizeof(VM::System);
    }
};


template <>
struct AllocFormatTraits<AllocFormat::System>
{
    AllocFormatTraits() = delete;
    typedef VM::System Type;
};


} // namespace GC
} // namespace Whisper



#endif // WHISPER__VM__SYSTEM_SPECIALIZATIONS_HPP

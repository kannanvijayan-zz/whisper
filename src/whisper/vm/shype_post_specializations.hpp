#ifndef WHISPER__VM__SHYPE_POST_SPECIALIZATIONS_HPP
#define WHISPER__VM__SHYPE_POST_SPECIALIZATIONS_HPP

#include "vm/core.hpp"

#include <cstring>
#include <new>

//
// GC-Specializations for String
//
namespace Whisper {
namespace GC {


template <>
struct AllocFormatTraits<AllocFormat::RootShype>
{
    AllocFormatTraits() = delete;
    typedef VM::RootShype Type;
};

template <>
struct AllocFormatTraits<AllocFormat::AddSlotShype>
{
    AllocFormatTraits() = delete;
    typedef VM::AddSlotShype Type;
};


template <>
struct TraceTraits<VM::RootShype>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::RootShype &rootShype,
                     const void *start, const void *end)
    {
        rootShype.parent_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::RootShype &rootShype,
                       const void *start, const void *end)
    {
        // Scan the path string.
        rootShype.parent_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::AddSlotShype>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::AddSlotShype &addSlotShype,
                     const void *start, const void *end)
    {
        addSlotShype.parent_.scan(scanner, start, end);
        addSlotShype.name_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::AddSlotShype &addSlotShype,
                       const void *start, const void *end)
    {
        addSlotShype.parent_.update(updater, start, end);
        addSlotShype.name_.update(updater, start, end);
    }
};



} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__SHYPE_POST_SPECIALIZATIONS_HPP

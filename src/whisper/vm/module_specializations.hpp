#ifndef WHISPER__VM__MODULE_SPECIALIZATIONS_HPP
#define WHISPER__VM__MODULE_SPECIALIZATIONS_HPP


#include "vm/core.hpp"
#include "vm/module.hpp"

#include <new>

namespace Whisper {
namespace GC {

//
// Specialize Module::Entry for TraceTraits and FieldTraits
//
template <>
struct TraceTraits<VM::Module::Entry>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::Module::Entry &entry,
                     const void *start, const void *end)
    {
        entry.name_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::Module::Entry &entry,
                       const void *start, const void *end)
    {
        entry.name_.update(updater, start, end);
    }
};

template <>
struct FieldTraits<VM::Module::Entry>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};


} // namespace GC
} // namespace Whisper


// Array specializations
WH_VM__DEF_SIMPLE_ARRAY_TRAITS(VM::Module::Entry, ModuleBindingsArray)


namespace Whisper {
namespace GC {


//
// Specialize Module for HeapTraits and TraceTraits
//

template <>
struct HeapTraits<VM::Module>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::Module;

    template <typename... Args>
    static uint32_t SizeOf(Args... rest) {
        return sizeof(VM::Module);
    }
};


template <>
struct AllocFormatTraits<AllocFormat::Module>
{
    AllocFormatTraits() = delete;
    typedef VM::Module Type;
};


template <>
struct TraceTraits<VM::Module>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::Module &module,
                     const void *start, const void *end)
    {
        module.sourceFiles_.scan(scanner, start, end);
        module.bindings_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::Module &module,
                       const void *start, const void *end)
    {
        module.sourceFiles_.update(updater, start, end);
        module.bindings_.update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper



#endif // WHISPER__VM__MODULE_SPECIALIZATIONS_HPP

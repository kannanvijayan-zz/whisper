#ifndef WHISPER__VM__LEXICAL_NAMESPACE_SPECIALIZATIONS_HPP
#define WHISPER__VM__LEXICAL_NAMESPACE_SPECIALIZATIONS_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"

#include <new>

namespace Whisper {
namespace GC {


template <>
struct TraceTraits<VM::LexicalNamespace::Entry>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::LexicalNamespace::Entry &entry,
                     const void *start, const void *end)
    {
        entry.name_.scan(scanner, start, end);
        entry.defn_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::LexicalNamespace::Entry &entry,
                       const void *start, const void *end)
    {
        entry.name_.update(updater, start, end);
        entry.defn_.update(updater, start, end);
    }
};

template <>
struct FieldTraits<VM::LexicalNamespace::Entry>
{
    FieldTraits() = delete;
    static constexpr bool Specialized = true;
};


} // namespace GC
} // namespace Whisper


// Array specializations
WH_VM__DEF_SIMPLE_ARRAY_TRAITS(VM::LexicalNamespace::Entry,
                               LexicalNamespaceBindingsArray)


namespace Whisper {
namespace GC {


//
// Specialize LexicalNamespace for HeapTraits and TraceTraits
//

template <typename... Args>
uint32_t
HeapTraits<VM::LexicalNamespace>::SizeOf(Args... rest)
{
        return sizeof(VM::LexicalNamespace);
}


template <>
struct AllocFormatTraits<AllocFormat::LexicalNamespace>
{
    AllocFormatTraits() = delete;
    typedef VM::LexicalNamespace Type;
};


template <>
struct TraceTraits<VM::LexicalNamespace>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::LexicalNamespace &lexNs,
                     const void *start, const void *end)
    {
        lexNs.parent_.scan(scanner, start, end);
        lexNs.bindings_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::LexicalNamespace &lexNs,
                       const void *start, const void *end)
    {
        lexNs.parent_.update(updater, start, end);
        lexNs.bindings_.update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__LEXICAL_NAMESPACE_SPECIALIZATIONS_HPP

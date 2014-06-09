#ifndef WHISPER__VM__SOURCE_FILE_SPECIALIZATIONS_HPP
#define WHISPER__VM__SOURCE_FILE_SPECIALIZATIONS_HPP

#include "vm/core.hpp"

#include <new>

//
// GC-Specializations for SourceFile
//
namespace Whisper {
namespace GC {


template <>
struct HeapTraits<VM::SourceFile>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::SourceFile;

    // All constructor signatures for Array take the length as the
    // first argument.
    template <typename... Args>
    static uint32_t SizeOf(Args... rest) {
        return sizeof(VM::SourceFile);
    }
};


template <>
struct AllocFormatTraits<AllocFormat::SourceFile>
{
    AllocFormatTraits() = delete;
    typedef VM::SourceFile Type;
};


template <>
struct TraceTraits<VM::SourceFile>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::SourceFile &sourceFile,
                     const void *start, const void *end)
    {
        // Scan the path string.
        sourceFile.path_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::SourceFile &sourceFile,
                       const void *start, const void *end)
    {
        // Scan the path string.
        sourceFile.path_.update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__SOURCE_FILE_SPECIALIZATIONS_HPP

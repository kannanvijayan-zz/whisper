#ifndef WHISPER__VM__SOURCE_FILE_HPP
#define WHISPER__VM__SOURCE_FILE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"

#include <new>

namespace Whisper {
namespace VM {

//
// A SourceFile contains mappings from symbols to the location within the
// file that contains the symbol definition.
//
class SourceFile
{
    friend struct TraceTraits<SourceFile>;

  private:
    HeapField<String *> path_;

  public:
    SourceFile(String *path)
      : path_(path)
    {
        WH_ASSERT(path != nullptr);
    }

    String *path() const {
        return path_;
    }
};


} // namespace VM

//
// GC Specializations
//


template <>
struct HeapTraits<VM::SourceFile>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = HeapFormat::SourceFile;
    static constexpr bool VarSized = false;
};


template <>
struct HeapFormatTraits<HeapFormat::SourceFile>
{
    HeapFormatTraits() = delete;
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


} // namespace Whisper


#endif // WHISPER__VM__SOURCE_FILE_HPP

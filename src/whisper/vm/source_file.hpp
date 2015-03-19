#ifndef WHISPER__VM__SOURCE_FILE_HPP
#define WHISPER__VM__SOURCE_FILE_HPP


#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/string.hpp"
#include "vm/packed_syntax_tree.hpp"

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
    HeapField<PackedSyntaxTree *> syntaxTree_;

  public:
    SourceFile(Handle<String *> path)
      : path_(path),
        syntaxTree_(nullptr)
    {
        WH_ASSERT(path != nullptr);
    }

    static Result<SourceFile *>Create(AllocationContext acx,
                                      Handle<String *> path);

    String *path() const {
        return path_;
    }

    bool hasSyntaxTree() const {
        return syntaxTree_.get() != nullptr;
    }
    PackedSyntaxTree *syntaxTree() const {
        WH_ASSERT(hasSyntaxTree());
        return syntaxTree_;
    }
};


} // namespace VM

//
// GC Specializations
//

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

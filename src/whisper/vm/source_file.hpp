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
    HeapField<ModuleObject *> scope_;
    HeapField<ScriptedFunction *> func_;

  public:
    SourceFile(String *path)
      : path_(path),
        syntaxTree_(nullptr),
        scope_(nullptr),
        func_(nullptr)
    {
        WH_ASSERT(path != nullptr);
    }

    static Result<SourceFile *> Create(AllocationContext acx,
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
    static Result<PackedSyntaxTree *> ParseSyntaxTree(
            ThreadContext *cx, Handle<SourceFile *> sourceFile);

    bool hasScope() const {
        return scope_.get() != nullptr;
    }
    ModuleObject *scope() const {
        WH_ASSERT(hasScope());
        return scope_;
    }
    static Result<ModuleObject *> CreateScope(
            ThreadContext *cx, Handle<SourceFile *> sourceFile);

    bool hasFunc() const {
        return func_.get() != nullptr;
    }
    ScriptedFunction *func() const {
        WH_ASSERT(hasFunc());
        return func_;
    }
    static Result<ScriptedFunction *> CreateFunc(
            ThreadContext *cx,
            Handle<SourceFile *> sourceFile,
            Handle<GlobalObject *> global);

  private:
    void setSyntaxTree(PackedSyntaxTree *tree) {
        WH_ASSERT(!hasSyntaxTree());
        syntaxTree_.set(tree, this);
    }
    void setFunc(ScriptedFunction *func) {
        WH_ASSERT(!hasFunc());
        func_.set(func, this);
    }
};


} // namespace VM

//
// GC Specializations
//

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
        if (sourceFile.syntaxTree_.get() != nullptr)
            sourceFile.syntaxTree_.scan(scanner, start, end);
        if (sourceFile.func_.get() != nullptr)
            sourceFile.func_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::SourceFile &sourceFile,
                       const void *start, const void *end)
    {
        // Scan the path string.
        sourceFile.path_.update(updater, start, end);
        if (sourceFile.syntaxTree_.get() != nullptr)
            sourceFile.syntaxTree_.update(updater, start, end);
        if (sourceFile.func_.get() != nullptr)
            sourceFile.func_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SOURCE_FILE_HPP

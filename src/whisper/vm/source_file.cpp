
#include "allocators.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/parser.hpp"
#include "parser/packed_writer.hpp"
#include "runtime_inlines.hpp"
#include "vm/function.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/global_scope.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<SourceFile *>
SourceFile::Create(AllocationContext acx, Handle<String *> path)
{
    return acx.create<SourceFile>(path);
}

/* static */ Result<PackedSyntaxTree *>
SourceFile::ParseSyntaxTree(ThreadContext *cx, Handle<SourceFile *> sourceFile)
{
    if (sourceFile->hasSyntaxTree())
        return OkVal(sourceFile->syntaxTree());

    // Load the file.
    FileCodeSource inputFile(sourceFile->path()->c_chars());
    if (inputFile.hasError()) {
        SpewParserError("Could not open input file for reading: %s",
                        sourceFile->path()->c_chars());
        cx->setError(RuntimeError::SyntaxParseFailed);
        return ErrorVal();
    }

    // Tokenize and parse it.
    BumpAllocator allocator;
    STLBumpAllocator<uint8_t> wrappedAllocator(allocator);

    Tokenizer tokenizer(inputFile);
    Parser parser(wrappedAllocator, tokenizer);
    FileNode *fileNode = parser.parseFile();
    if (!fileNode) {
        WH_ASSERT(parser.hasError());
        SpewParserError("Error during parse: %s", parser.error());
        return ErrorVal();
    }

    AllocationContext acx = cx->inTenured();

    // Write out the syntax tree in packed format.
    Local<AST::PackedWriter> packedWriter(cx,
        PackedWriter(
            STLBumpAllocator<uint32_t>(wrappedAllocator),
            tokenizer.sourceReader(),
            acx));
    packedWriter->writeNode(fileNode);

    // Create the packed syntax tree.
    ArrayHandle<uint32_t> buffer = packedWriter->buffer();
    ArrayHandle<Box> constPool = packedWriter->constPool();

    Local<PackedSyntaxTree *> packedSt(cx);
    if (!packedSt.setResult(PackedSyntaxTree::Create(acx, buffer, constPool)))
        return ErrorVal();

    sourceFile->setSyntaxTree(packedSt);
    return OkVal(sourceFile->syntaxTree());
}

/* static */ Result<ModuleScope *>
SourceFile::CreateScope(ThreadContext *cx, Handle<SourceFile *> sourceFile)
{
    AllocationContext acx = cx->inTenured();

    // Ensure we have a packed syntax tree.
    Local<PackedSyntaxTree *> pst(cx);
    if (!pst.setResult(SourceFile::ParseSyntaxTree(cx, sourceFile)))
        return ErrorVal();

    // Create a module object for the file.  The caller scope for
    // the module is the global.
    Local<GlobalScope *> global(cx, cx->global());
    Local<ModuleScope *> module(acx);
    if (!module.setResult(ModuleScope::Create(acx, global)))
        return ErrorVal();

    // install the module.
    sourceFile->scope_.set(module, sourceFile.get());

    return OkVal(module.get());
}


} // namespace VM
} // namespace Whisper

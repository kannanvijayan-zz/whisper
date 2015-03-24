
#include "allocators.hpp"
#include "parser/code_source.hpp"
#include "parser/tokenizer.hpp"
#include "parser/parser.hpp"
#include "parser/packed_writer.hpp"
#include "runtime_inlines.hpp"
#include "vm/function.hpp"
#include "vm/source_file.hpp"

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
        return Result<PackedSyntaxTree *>::Value(sourceFile->syntaxTree());

    // Load the file.
    FileCodeSource inputFile(sourceFile->path()->c_chars());
    if (inputFile.hasError()) {
        SpewParserError("Could not open input file for reading: %s",
                        sourceFile->path()->c_chars());
        cx->setError(RuntimeError::SyntaxParseFailed);
        return Result<PackedSyntaxTree *>::Error();
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
        return Result<PackedSyntaxTree *>::Error();
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
        return Result<PackedSyntaxTree *>::Error();

    sourceFile->setSyntaxTree(packedSt);
    return Result<PackedSyntaxTree *>::Value(sourceFile->syntaxTree());
}

/* static */ Result<ScriptedFunction *>
SourceFile::CreateFunc(
            ThreadContext *cx,
            Handle<SourceFile *> sourceFile,
            Handle<GlobalObject *> global)
{
    if (sourceFile->hasFunc())
        return Result<ScriptedFunction *>::Value(sourceFile->func());

    // Ensure we have a packed syntax tree.
    Local<PackedSyntaxTree *> pst(cx);
    if (!pst.setResult(SourceFile::ParseSyntaxTree(cx, sourceFile)))
        return Result<ScriptedFunction *>::Error();

    AllocationContext acx = cx->inTenured();

    // Create a new SyntaxTreeFrament pointing to the File node.
    Local<SyntaxTreeFragment *> defn(cx);
    if (!defn.setResult(SyntaxTreeFragment::Create(acx, pst, 0)))
        return Result<ScriptedFunction *>::Error();

    // Createa a new scripted function.
    Local<ScriptedFunction *> func(cx);
    if (!func.setResult(ScriptedFunction::Create(acx, defn,
                            Handle<ScopeObject *>::Convert(global),
                            /* isOperative = */ false)))
    {
        return Result<ScriptedFunction *>::Error();
    }

    // Save scripted function to source file.
    sourceFile->setFunc(func);
    return Result<ScriptedFunction *>::Value(func);
}


} // namespace VM
} // namespace Whisper


#include "interp/bytecode_generator.hpp"

#include "runtime_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace Interp {


BytecodeGenerator::BytecodeGenerator(
        RunContext *cx, const STLBumpAllocator<uint8_t> &allocator,
        AST::ProgramNode *node, bool strict)
  : cx_(cx),
    allocator_(allocator),
    node_(node),
    strict_(strict)
{
    WH_ASSERT(node_);
}

bool
BytecodeGenerator::hasError() const
{
    return error_ != nullptr;
}

const char *
BytecodeGenerator::error() const
{
    WH_ASSERT(hasError());
    return error_;
}

VM::Bytecode *
BytecodeGenerator::generateBytecode()
{
    VM::RootedBytecode bytecode(cx_);

    // Scan bytecode initially to calculate info and check for errors.
    try {
        generate(bytecode);
    } catch (BytecodeGeneratorError &exc) {
        WH_ASSERT(hasError());
        return nullptr;
    }

    WH_ASSERT(bytecodeSize_ > 0);

    // Bytecode scanning worked out.  Allocate a bytecode object.
    bytecode = cx_->create<VM::Bytecode>(true, bytecodeSize_);
    WH_ASSERT(bytecode);

    // Fill the bytecode object.
    generate(bytecode);

    return bytecode;
}

void
BytecodeGenerator::generate(VM::HandleBytecode bytecode)
{
    for (AST::SourceElementNode *elem : node_->sourceElements()) {
        if (elem->isFunctionDeclaration())
            emitError("Cannot handle function declarations yet.");

        // Otherwise, it must be a statement.
        WH_ASSERT(elem->isStatement());
        if (elem->isExpressionStatement()) {
            generateExpressionStatement(bytecode,
                                        elem->toExpressionStatement());
            continue;
        }

        emitError("Cannot handle this syntax node yet.");
    }
}

void
BytecodeGenerator::generateExpressionStatement(
        VM::HandleBytecode bytecode, AST::ExpressionStatementNode *exptStmt)
{
}

void
BytecodeGenerator::emitError(const char *msg)
{
    WH_ASSERT(!hasError());
    error_ = msg;
    throw BytecodeGeneratorError();
}


} // namespace Interp
} // namespace Whisper

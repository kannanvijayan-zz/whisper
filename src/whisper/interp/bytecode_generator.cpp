
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
    // Scan bytecode initially to calculate info and check for errors.
    try {
        scan();
    } catch (BytecodeGeneratorError &exc) {
        WH_ASSERT(hasError());
        return nullptr;
    }

    WH_ASSERT(bytecodeSize_ > 0);

    // Bytecode scanning worked out.  Allocate a bytecode object.
    VM::RootedBytecode bc(cx_, cx_->create<VM::Bytecode>(true, bytecodeSize_));
    WH_ASSERT(bc);

    // Fill the bytecode object.
    fill(bc);
    return bc;
}

void
BytecodeGenerator::scan()
{
}

void
BytecodeGenerator::fill(VM::HandleBytecode bytecode)
{
}


} // namespace Interp
} // namespace Whisper

#ifndef WHISPER__INTERP__BYTECODEGEN_HPP
#define WHISPER__INTERP__BYTECODEGEN_HPP

#include "common.hpp"
#include "debug.hpp"
#include "allocators.hpp"
#include "runtime.hpp"
#include "parser/syntax_tree.hpp"
#include "vm/bytecode.hpp"

//
// The bytecode generator converts a syntax tree into interpretable
// bytecode.
//

namespace Whisper {
namespace Interp {


class BytecodeGeneratorError {
  friend class BytecodeGenerator;
  private:
    inline BytecodeGeneratorError() {}
};

class BytecodeGenerator
{
  private:
    // The RunContext for the generator.
    RunContext *cx_;

    // The allocator to use during parsing.
    STLBumpAllocator<uint8_t> allocator_;

    // The syntax tree code is being generated for.
    AST::ProgramNode *node_;

    // Whether to start with strict mode.
    bool strict_;

    // Error message.
    const char *error_ = nullptr;

    /// Generated information. ///

    // The calculated size of the bytecode.
    uint32_t bytecodeSize_ = 0;

    // The maximum stack depth.
    uint32_t maxStackDepth_ = 0;

  public:
    BytecodeGenerator(RunContext *cx,
                      const STLBumpAllocator<uint8_t> &allocator,
                      AST::ProgramNode *node, bool strict);

    bool hasError() const;
    const char *error() const;

    VM::Bytecode *generateBytecode();

  private:
    void generate(VM::HandleBytecode bytecode);
    void generateExpressionStatement(VM::HandleBytecode bytecode,
                                     AST::ExpressionStatementNode *exptStmt);

    void emitError(const char *msg);
};


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODEGEN_HPP

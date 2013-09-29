#ifndef WHISPER__INTERP__BYTECODEGEN_HPP
#define WHISPER__INTERP__BYTECODEGEN_HPP

#include "common.hpp"
#include "debug.hpp"
#include "allocators.hpp"
#include "runtime.hpp"
#include "parser/syntax_tree.hpp"
#include "parser/syntax_annotations.hpp"
#include "vm/bytecode.hpp"
#include "interp/bytecode_defn.hpp"
#include "interp/bytecode_ops.hpp"

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

    // The syntax annotator used to analyze the syntax tree.
    const AST::SyntaxAnnotator &annotator_;

    // Whether to start with strict mode.
    bool strict_;

    // Bytecode.
    VM::RootedBytecode bytecode_;

    // Error message.
    const char *error_ = nullptr;

    /// Generated information. ///

    // The calculated size of the bytecode.
    uint32_t bytecodeSize_ = 0;

    // The maximum stack depth.
    uint32_t maxStackDepth_ = 0;


    /// Intermediate state. ///

    // Flag controlling whether stack depth should be calculated.
    bool calculateStackDepth_ = false;

    // The current bytecode size.
    uint32_t currentBytecodeSize_ = 0;

    // The current stack depth.
    uint32_t currentStackDepth_ = 0;

  public:
    BytecodeGenerator(RunContext *cx,
                      const STLBumpAllocator<uint8_t> &allocator,
                      AST::ProgramNode *node,
                      AST::SyntaxAnnotator &annotator,
                      bool strict);

    bool hasError() const;
    const char *error() const;

    VM::Bytecode *generateBytecode();

  private:
    void generate();
    void generateExpressionStatement(AST::ExpressionStatementNode *exprStmt);
    void generateExpression(AST::ExpressionNode *expr,
                            const OperandLocation &outputLocation);

    bool getAddressableLocation(AST::ExpressionNode *expr,
                                OperandLocation &location);


    void emitPushInt32(int32_t value);
    void emitUnaryOp(AST::BaseUnaryExpressionNode *expr,
                     const OperandLocation &inputLocation,
                     const OperandLocation &outputLocation);
    void emitBinaryOp(AST::BaseBinaryExpressionNode *expr,
                      const OperandLocation &lhsLocation,
                      const OperandLocation &rhsLocation,
                      const OperandLocation &outputLocation);
    void emitPop(uint16_t num=1);
    void emitOperandLocation(const OperandLocation &location);
    void emitOp(Opcode op);
    void emitConstantOperand(uint32_t idx);
    void emitArgumentOperand(uint32_t idx);
    void emitLocalOperand(uint32_t idx);
    void emitStackOperand(uint32_t idx);
    void emitImmediateUnsignedOperand(uint32_t val);
    void emitImmediateSignedOperand(int32_t val);

    void emitIndexedOperand(OperandSpace space, uint32_t idx);
    void emitByte(uint8_t byte);
    
    void emitError(const char *msg);
};


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODEGEN_HPP

#ifndef WHISPER__INTERP__INTERPRETER_HPP
#define WHISPER__INTERP__INTERPRETER_HPP

#include "common.hpp"
#include "runtime.hpp"
#include "vm/script.hpp"
#include "vm/stack_frame.hpp"
#include "interp/bytecode_ops.hpp"

namespace Whisper {
namespace Interp {


/**
 * Start a top-level interpretation.
 */
bool InterpretScript(RunContext *cx, Handle<VM::Script *> script);


/**
 * Interpreter holds the active runtime state for a running
 * interpreter.  It's a stack-instantiated object.
 */
class Interpreter
{
  private:
    // The current execution context.
    RunContext *cx_;

    // The current active stack frame.
    Root<VM::StackFrame *> frame_;

    // The script and bytecode being executed.
    Root<VM::Script *> script_;
    Root<VM::Bytecode *> bytecode_;

    // Pc info.
    const uint8_t *pc_;
    const uint8_t *pcEnd_;

  public:
    Interpreter(RunContext *cx, Handle<VM::StackFrame *> frame);

    bool interpret();

  private:
    Value readOperand(const OperandLocation &loc);

    bool interpretStop(Opcode op, int32_t *opBytes);
    bool interpretPushInt(Opcode op, int32_t *opBytes);
    bool interpretAdd(Opcode op, int32_t *opBytes);

    void readBinaryOperandLocations(Opcode op, Opcode baseOp,
                                    OperandLocation *lhsLoc,
                                    OperandLocation *rhsLoc,
                                    OperandLocation *outLoc,
                                    int32_t *opBytes);

    void readBinaryOperandValues(Opcode op, Opcode baseOp,
                                 MutHandle<Value> lhs,
                                 MutHandle<Value> rhs,
                                 OperandLocation *outLoc,
                                 int32_t *opBytes);
};


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__INTERPRETER_HPP

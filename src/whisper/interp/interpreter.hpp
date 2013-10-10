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
bool InterpretScript(RunContext *cx, VM::HandleScript script);


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
    VM::RootedStackFrame frame_;

    // The script and bytecode being executed.
    VM::RootedScript script_;
    VM::RootedBytecode bytecode_;

    // Pc info.
    const uint8_t *pc_;
    const uint8_t *pcEnd_;

  public:
    Interpreter(RunContext *cx, VM::HandleStackFrame frame);

    bool interpret();

  private:
    Value readOperand(const OperandLocation &loc);

    bool interpretPushInt(Opcode op, uint32_t *opBytes);
    bool interpretIf(Opcode op, uint32_t *opBytes);
};


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__INTERPRETER_HPP

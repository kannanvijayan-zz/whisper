
#include "common.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/stack_frame.hpp"
#include "vm/bytecode.hpp"
#include "interp/interpreter.hpp"

namespace Whisper {
namespace Interp {

bool
InterpretScript(RunContext *cx, VM::HandleScript script)
{
    WH_ASSERT(script->isTopLevel());

    // Ensure that no stack frames are currently pushed on the RunContext.

    // Create a new stack frame for this script.
    VM::StackFrame::Config config;
    config.maxStackDepth = script->maxStackDepth();
    config.numActualArgs = 0;

    uint32_t size = VM::StackFrame::CalculateSize(config);
    VM::RootedStackFrame stackFrame(cx,
        cx->createSized<VM::StackFrame>(true, size, script, config));

    // Register the stack frame with the runtime.
    cx->registerTopStackFrame(stackFrame);

    Interpreter interp(cx, stackFrame);
    return interp.interpret();
}

Interpreter::Interpreter(RunContext *cx, VM::HandleStackFrame frame)
  : cx_(cx),
    frame_(cx, frame),
    script_(cx, frame_->script()),
    bytecode_(cx_, script_->bytecode()),
    pc_(bytecode_->dataAt(frame_->pcOffset())),
    pcEnd_(bytecode_->dataEnd())
{}

bool
Interpreter::interpret()
{
    uint32_t opBytes = 0;

    for (;;) {
        // Move to next op.
        pc_ += opBytes;
        WH_ASSERT(pc_ < pcEnd_);

        Opcode op;
        opBytes = Interp::ReadOpcode(pc_, pcEnd_, &op);

        switch (op) {
          case Opcode::Nop:
            break;

          case Opcode::Pop:
            WH_ASSERT(frame_->curStackDepth() > 0);
            frame_->popValue();
            break;

          case Opcode::PushInt8:
          case Opcode::PushInt16:
          case Opcode::PushInt24:
          case Opcode::PushInt32:
            if (!interpretPushInt(op, &opBytes))
                return false;
            break;

          case Opcode::Ret_S:
            // TODO: implement
            WH_UNREACHABLE("Unhandled op.");
            break;

          case Opcode::Ret_V:
            // TODO: implement
            WH_UNREACHABLE("Unhandled op.");
            break;

          case Opcode::If_S:
          case Opcode::If_V:
          case Opcode::IfNot_S:
          case Opcode::IfNot_V:
            if (!interpretIf(op, &opBytes))
                return false;
            break;

          case Opcode::Add_SSS: // E
          case Opcode::Add_SSV: // V
          case Opcode::Add_SVS: // V
          case Opcode::Add_SVV: // VV
          case Opcode::Add_VSS: // V
          case Opcode::Add_VSV: // VV
          case Opcode::Add_VVS: // VV
          case Opcode::Add_VVV: // VVV

          case Opcode::Sub_SSS: // E
          case Opcode::Sub_SSV: // V
          case Opcode::Sub_SVS: // V
          case Opcode::Sub_SVV: // VV
          case Opcode::Sub_VSS: // V
          case Opcode::Sub_VSV: // VV
          case Opcode::Sub_VVS: // VV
          case Opcode::Sub_VVV: // VVV

          case Opcode::Mul_SSS: // E
          case Opcode::Mul_SSV: // V
          case Opcode::Mul_SVS: // V
          case Opcode::Mul_SVV: // VV
          case Opcode::Mul_VSS: // V
          case Opcode::Mul_VSV: // VV
          case Opcode::Mul_VVS: // VV
          case Opcode::Mul_VVV: // VVV

          case Opcode::Div_SSS: // E
          case Opcode::Div_SSV: // V
          case Opcode::Div_SVS: // V
          case Opcode::Div_SVV: // VV
          case Opcode::Div_VSS: // V
          case Opcode::Div_VSV: // VV
          case Opcode::Div_VVS: // VV
          case Opcode::Div_VVV: // VVV

          case Opcode::Mod_SSS: // E
          case Opcode::Mod_SSV: // V
          case Opcode::Mod_SVS: // V
          case Opcode::Mod_SVV: // VV
          case Opcode::Mod_VSS: // V
          case Opcode::Mod_VSV: // VV
          case Opcode::Mod_VVS: // VV
          case Opcode::Mod_VVV: // VVV

          case Opcode::Neg_SS: // E
          case Opcode::Neg_SV: // V
          case Opcode::Neg_VS: // V
          case Opcode::Neg_VV: // VV

          default:
            WH_UNREACHABLE("Unhandled op.");
        }
    }

    return false;
}


Value
Interpreter::readOperand(const OperandLocation &location)
{
    switch (location.space()) {
      case OperandSpace::Constant:
        WH_UNREACHABLE("Constant is unhandled!");
        return UndefinedValue();

      case OperandSpace::Argument:
        return frame_->actualArg(location.argumentIndex());

      case OperandSpace::Local:
        return frame_->actualArg(location.localIndex());

      case OperandSpace::Stack:
        return frame_->peekValue(location.stackIndex());

      case OperandSpace::Immediate:
        if (location.isSigned()) {
            return IntegerValue(location.signedValue());
        } else {
            WH_ASSERT(location.unsignedValue() < 0xFu);
            return IntegerValue(location.unsignedValue());
        }

      case OperandSpace::StackTop:
        {
            Value result = frame_->peekValue(location.stackIndex());
            frame_->popValue();
            return result;
        }

      default:
        WH_UNREACHABLE("Invalid operand kind.");
    }
    return UndefinedValue();
}


bool
Interpreter::interpretPushInt(Opcode op, uint32_t *opBytes)
{
    OpcodeFormat fmt = OpcodeFormat::E;
    switch (op) {
      case Opcode::PushInt8:
        fmt = OpcodeFormat::I1;
        break;

      case Opcode::PushInt16:
        fmt = OpcodeFormat::I2;
        break;

      case Opcode::PushInt24:
        fmt = OpcodeFormat::I3;
        break;

      case Opcode::PushInt32:
        fmt = OpcodeFormat::I4;
        break;

      default:
        WH_UNREACHABLE("Invalid op.");
    }

    WH_ASSERT(GetOpcodeFormat(op) == fmt);

    OperandLocation oploc;
    *opBytes += ReadOperandLocation(pc_, pcEnd_, fmt, 0, &oploc);
    WH_ASSERT(oploc.isImmediate() && oploc.isSigned());

    frame_->pushValue(IntegerValue(oploc.signedValue()));

    return true;
}

bool
Interpreter::interpretIf(Opcode op, uint32_t *opBytes)
{
    bool negate = false;
    OpcodeFormat fmt = OpcodeFormat::E;
    switch (op) {
      case Opcode::IfNot_S:
        negate = true;
      case Opcode::If_S:
        break;

      case Opcode::IfNot_V:
        negate = true;
      case Opcode::If_V:
        fmt = OpcodeFormat::V;
        break;

      default:
        WH_UNREACHABLE("Invalid op.");
    }

    WH_ASSERT(GetOpcodeFormat(op) == fmt);

    OperandLocation oploc;
    if (fmt == OpcodeFormat::E)
        oploc = OperandLocation::StackTop();
    else
        *opBytes += ReadOperandLocation(pc_, pcEnd_, fmt, 0, &oploc);

    RootedValue v(cx_, readOperand(oploc));
    if (negate) {
        // negate value v.
    }

    // TODO: read operand, branch on result.

    return true;
}


} // namespace Interp
} // namespace Whisper

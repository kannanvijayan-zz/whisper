
#include "common.hpp"
#include "spew.hpp"
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
InterpretScript(RunContext *cx, Handle<VM::Script *> script)
{
    WH_ASSERT(script->isTopLevel());

    // Ensure that no stack frames are currently pushed on the RunContext.

    // Create a new stack frame for this script.
    VM::StackFrame::Config config;
    config.numPassedArgs = 0;
    config.numArgs = 0;
    config.numLocals = 0;
    config.maxStackDepth = script->maxStackDepth();

    uint32_t size = VM::StackFrame::CalculateSize(config);
    Root<VM::StackFrame *> stackFrame(cx,
        cx->createSized<VM::StackFrame>(size, script, config));
    if (!stackFrame)
        return false;

    // Register the stack frame with the runtime.
    cx->registerTopStackFrame(stackFrame);

    Interpreter interp(cx, stackFrame);
    return interp.interpret();
}

Interpreter::Interpreter(RunContext *cx, Handle<VM::StackFrame *> frame)
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
    int32_t opBytes = 0;

    for (;;) {
        // Move to next op.
        pc_ += opBytes;
        WH_ASSERT(pc_ >= bytecode_->data());

        // If natural end of interpretation reached, stop.
        if (pc_ == pcEnd_)
            return true;

        Opcode op;
        opBytes = Interp::ReadOpcode(pc_, pcEnd_, &op);
        SpewInterpOpNote("Op %s", OpcodeString(op));

        switch (op) {
          case Opcode::Nop:
            break;

          case Opcode::Pop:
            WH_ASSERT(frame_->stackDepth() > 0);
            frame_->popStack();
            break;

          case Opcode::Stop:
            if (!interpretStop(op, &opBytes))
                return false;
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
            WH_UNREACHABLE("Unhandled op Ret_S.");
            break;

          case Opcode::Ret_V:
            // TODO: implement
            WH_UNREACHABLE("Unhandled op Ret_V.");
            break;

          case Opcode::Add_SSS: // E
          case Opcode::Add_SSV: // V
          case Opcode::Add_SVS: // V
          case Opcode::Add_SVV: // VV
          case Opcode::Add_VSS: // V
          case Opcode::Add_VSV: // VV
          case Opcode::Add_VVS: // VV
          case Opcode::Add_VVV: // VVV
            if (!interpretAdd(op, &opBytes))
                return false;
            break;

          case Opcode::Sub_SSS: // E
          case Opcode::Sub_SSV: // V
          case Opcode::Sub_SVS: // V
          case Opcode::Sub_SVV: // VV
          case Opcode::Sub_VSS: // V
          case Opcode::Sub_VSV: // VV
          case Opcode::Sub_VVS: // VV
          case Opcode::Sub_VVV: // VVV
            if (!interpretSub(op, &opBytes))
                return false;
            break;

          case Opcode::Mul_SSS: // E
          case Opcode::Mul_SSV: // V
          case Opcode::Mul_SVS: // V
          case Opcode::Mul_SVV: // VV
          case Opcode::Mul_VSS: // V
          case Opcode::Mul_VSV: // VV
          case Opcode::Mul_VVS: // VV
          case Opcode::Mul_VVV: // VVV
            if (!interpretMul(op, &opBytes))
                return false;
            break;

          case Opcode::Div_SSS: // E
          case Opcode::Div_SSV: // V
          case Opcode::Div_SVS: // V
          case Opcode::Div_SVV: // VV
          case Opcode::Div_VSS: // V
          case Opcode::Div_VSV: // VV
          case Opcode::Div_VVS: // VV
          case Opcode::Div_VVV: // VVV
            if (!interpretDiv(op, &opBytes))
                return false;
            break;

          case Opcode::Mod_SSS: // E
          case Opcode::Mod_SSV: // V
          case Opcode::Mod_SVS: // V
          case Opcode::Mod_SVV: // VV
          case Opcode::Mod_VSS: // V
          case Opcode::Mod_VSV: // VV
          case Opcode::Mod_VVS: // VV
          case Opcode::Mod_VVV: // VVV
            if (!interpretMod(op, &opBytes))
                return false;
            break;

          case Opcode::Neg_SS: // E
          case Opcode::Neg_SV: // V
          case Opcode::Neg_VS: // V
          case Opcode::Neg_VV: // VV
            if (!interpretNeg(op, &opBytes))
                return false;
            break;

          default:
            WH_UNREACHABLE("Unhandled op.");
        }
    }

    return false;
}


Value
Interpreter::readOperand(const OperandLocation &loc)
{
    switch (loc.space()) {
      case OperandSpace::Constant:
        WH_ASSERT(script_->constants());
        return script_->constants()->get(0);

      case OperandSpace::Argument:
        return frame_->getArg(loc.argumentIndex());

      case OperandSpace::Local:
        return frame_->getLocal(loc.localIndex());

      case OperandSpace::Stack:
        return frame_->peekStack(loc.stackIndex());

      case OperandSpace::Immediate:
        if (loc.isSigned()) {
            return Value::Int32(loc.signedValue());
        } else {
            WH_ASSERT(loc.unsignedValue() < 0xFu);
            return Value::Int32(loc.unsignedValue());
        }

      case OperandSpace::StackTop:
        {
            Value result = frame_->peekStack(0);
            frame_->popStack();
            return result;
        }

      default:
        WH_UNREACHABLE("Invalid operand kind.");
    }
    return Value::Undefined();
}

void
Interpreter::writeOperand(const OperandLocation &loc, const Value &val)
{
    switch (loc.space()) {
      case OperandSpace::Constant:
        WH_UNREACHABLE("Constant is not a valid write location!");
        break;

      case OperandSpace::Argument:
        frame_->setArg(loc.argumentIndex(), val);
        break;

      case OperandSpace::Local:
        frame_->setLocal(loc.localIndex(), val);
        break;

      case OperandSpace::Stack:
        frame_->pokeStack(loc.stackIndex(), val);

      case OperandSpace::Immediate:
        WH_UNREACHABLE("Immediate is not a valid write location!");
        break;

      case OperandSpace::StackTop:
        frame_->pushStack(val);
        break;

      default:
        WH_UNREACHABLE("Invalid operand kind.");
    }
}


bool
Interpreter::interpretStop(Opcode op, int32_t *opBytes)
{
    WH_ASSERT(frame_->stackDepth() == 0);
    VM::Script::Mode mode = script_->mode();

    // TopLevel scripts don't return value.
    if (mode == VM::Script::TopLevel)
        return true;

    WH_UNREACHABLE("Can't handle Eval and Function scripts yet.");
    return false;
}


bool
Interpreter::interpretPushInt(Opcode op, int32_t *opBytes)
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
    *opBytes += ReadOperandLocation(pc_ + *opBytes, pcEnd_, fmt, 0, &oploc);
    WH_ASSERT(oploc.isImmediate() && oploc.isSigned());

    frame_->pushStack(Value::Int32(oploc.signedValue()));

    return true;
}


bool
Interpreter::interpretAdd(Opcode op, int32_t *opBytes)
{
    Root<Value> lhs(cx_, Value::Undefined());
    Root<Value> rhs(cx_, Value::Undefined());
    OperandLocation outLoc;

    readBinaryOperandValues(op, Opcode::Add_SSS, &lhs, &rhs, &outLoc, opBytes);

    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();
        int32_t resultVal = lhsVal + rhsVal;

        // Check for int32 overflow.
        bool overflow = false;
        bool lhsSign = lhsVal >> 31;
        bool rhsSign = rhsVal >> 31;
        if (lhsSign == rhsSign) {
            bool resultSign = resultVal >> 31;
            if (resultSign != lhsSign)
                overflow = true;
        }

        if (!overflow) {
            SpewInterpOpNote("  Int32 add %d + %d => %d",
                             lhsVal, rhsVal, resultVal);
            writeOperand(outLoc, Value::Int32(resultVal));
            return true;
        }
    }

    WH_UNREACHABLE("Non-int32 add not implemented yet!");
    return false;
}


bool
Interpreter::interpretSub(Opcode op, int32_t *opBytes)
{
    Root<Value> lhs(cx_, Value::Undefined());
    Root<Value> rhs(cx_, Value::Undefined());
    OperandLocation outLoc;

    readBinaryOperandValues(op, Opcode::Sub_SSS, &lhs, &rhs, &outLoc, opBytes);

    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();
        int32_t resultVal = lhsVal - rhsVal;

        // Check for int32 overflow.
        bool overflow = false;
        if (rhsVal == INT32_MIN) {
            overflow = true;
        } else {
            bool lhsSign = lhsVal >> 31;
            bool negRhsSign = -rhsVal >> 31;
            if (lhsSign == negRhsSign) {
                bool resultSign = resultVal >> 31;
                if (resultSign != lhsSign)
                    overflow = true;
            }
        }

        if (!overflow) {
            SpewInterpOpNote("  Int32 sub %d - %d => %d",
                             lhsVal, rhsVal, resultVal);
            writeOperand(outLoc, Value::Int32(resultVal));
            return true;
        }
    }

    WH_UNREACHABLE("Non-int32 subtract not implemented yet!");
    return false;
}


static unsigned
NumSignificantBits(int32_t val)
{
    unsigned result = 0;
    while (val != 0 && val != -1) {
        result += 1;
        val >>= 1;
    }
    // one extra bit needed to represent sign.
    result += 1;
    return result;
}


bool
Interpreter::interpretMul(Opcode op, int32_t *opBytes)
{
    Root<Value> lhs(cx_, Value::Undefined());
    Root<Value> rhs(cx_, Value::Undefined());
    OperandLocation outLoc;

    readBinaryOperandValues(op, Opcode::Mul_SSS, &lhs, &rhs, &outLoc, opBytes);

    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();

        // Check number of significant bits in each number.
        unsigned lhsBits = NumSignificantBits(lhsVal);
        unsigned rhsBits = NumSignificantBits(rhsVal);

        // Check for int32 overflow.
        bool overflow = (lhsBits + rhsBits > 31);

        if (!overflow) {
            int32_t resultVal = lhsVal * rhsVal;
            SpewInterpOpNote("  Int32 mul %d * %d => %d",
                             lhsVal, rhsVal, resultVal);
            writeOperand(outLoc, Value::Int32(resultVal));
            return true;
        }
    }

    WH_UNREACHABLE("Non-int32 multiply not implemented yet!");
    return false;
}


bool
Interpreter::interpretDiv(Opcode op, int32_t *opBytes)
{
    Root<Value> lhs(cx_, Value::Undefined());
    Root<Value> rhs(cx_, Value::Undefined());
    OperandLocation outLoc;

    readBinaryOperandValues(op, Opcode::Div_SSS, &lhs, &rhs, &outLoc, opBytes);

    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();

        bool overflow = (lhsVal % rhsVal) != 0;

        if (!overflow) {
            int32_t resultVal = lhsVal / rhsVal;
            SpewInterpOpNote("  Int32 div %d / %d => %d",
                             lhsVal, rhsVal, resultVal);
            writeOperand(outLoc, Value::Int32(resultVal));
            return true;
        }
    }

    WH_UNREACHABLE("Non-int32 divide not implemented yet!");
    return false;
}


bool
Interpreter::interpretMod(Opcode op, int32_t *opBytes)
{
    Root<Value> lhs(cx_, Value::Undefined());
    Root<Value> rhs(cx_, Value::Undefined());
    OperandLocation outLoc;

    readBinaryOperandValues(op, Opcode::Mod_SSS, &lhs, &rhs, &outLoc, opBytes);

    if (lhs->isInt32() && rhs->isInt32()) {
        int32_t lhsVal = lhs->int32Value();
        int32_t rhsVal = rhs->int32Value();

        int32_t resultVal = lhsVal % rhsVal;
        SpewInterpOpNote("  Int32 mod %d %% %d => %d",
                         lhsVal, rhsVal, resultVal);
        writeOperand(outLoc, Value::Int32(resultVal));
        return true;
    }

    WH_UNREACHABLE("Non-int32 modulo not implemented yet!");
    return false;
}


bool
Interpreter::interpretNeg(Opcode op, int32_t *opBytes)
{
    Root<Value> input(cx_, Value::Undefined());
    OperandLocation outLoc;

    readUnaryOperandValues(op, Opcode::Neg_SS, &input, &outLoc, opBytes);

    if (input->isInt32()) {
        int32_t inputVal = input->int32Value();

        bool overflow = (inputVal == INT32_MIN);
        if (!overflow) {
            int32_t resultVal = -inputVal;
            SpewInterpOpNote("  Int32 neg -%d => %d", inputVal, resultVal);
            writeOperand(outLoc, Value::Int32(resultVal));
            return true;
        }
    }

    WH_UNREACHABLE("Non-int32 negate not implemented yet!");
    return false;
}


void
Interpreter::readBinaryOperandLocations(Opcode op, Opcode baseOp,
                                        OperandLocation *lhsLoc,
                                        OperandLocation *rhsLoc,
                                        OperandLocation *outLoc,
                                        int32_t *opBytes)
{
    unsigned opcodeOffset = OpcodeNumber(op) - OpcodeNumber(baseOp);
    WH_ASSERT(opcodeOffset < 8);
    bool lhsIsValue = opcodeOffset & (1 << 2);
    bool rhsIsValue = opcodeOffset & (1 << 1);
    bool outIsValue = opcodeOffset & (1 << 0);

    const OpcodeFormat V = OpcodeFormat::V;

    if (lhsIsValue)
        *opBytes += ReadOperandLocation(pc_ + *opBytes, pcEnd_, V, 0, lhsLoc);
    else
        *lhsLoc = OperandLocation::StackTop();

    if (rhsIsValue)
        *opBytes += ReadOperandLocation(pc_ + *opBytes, pcEnd_, V, 0, rhsLoc);
    else
        *rhsLoc = OperandLocation::StackTop();

    if (outIsValue)
        *opBytes += ReadOperandLocation(pc_ + *opBytes, pcEnd_, V, 0, outLoc);
    else
        *outLoc = OperandLocation::StackTop();
}


void
Interpreter::readBinaryOperandValues(Opcode op, Opcode baseOp,
                                     MutHandle<Value> lhs,
                                     MutHandle<Value> rhs,
                                     OperandLocation *outLoc,
                                     int32_t *opBytes)
{
    OperandLocation lhsLoc;
    OperandLocation rhsLoc;
    readBinaryOperandLocations(op, baseOp, &lhsLoc, &rhsLoc, outLoc, opBytes);

    // Read the operands.  Read the rhs first because if both lhs and
    // rhs are read from the StackTop, then they should be popped in
    // the right order (rhs, then lhs).
    rhs = readOperand(rhsLoc);
    lhs = readOperand(lhsLoc);
}


void
Interpreter::readUnaryOperandLocations(Opcode op, Opcode baseOp,
                                       OperandLocation *inLoc,
                                       OperandLocation *outLoc,
                                       int32_t *opBytes)
{
    unsigned opcodeOffset = OpcodeNumber(op) - OpcodeNumber(baseOp);
    WH_ASSERT(opcodeOffset < 4);
    bool inputIsValue = opcodeOffset & (1 << 2);
    bool outIsValue = opcodeOffset & (1 << 0);

    const OpcodeFormat V = OpcodeFormat::V;

    if (inputIsValue)
        *opBytes += ReadOperandLocation(pc_ + *opBytes, pcEnd_, V, 0, inLoc);
    else
        *inLoc = OperandLocation::StackTop();

    if (outIsValue)
        *opBytes += ReadOperandLocation(pc_ + *opBytes, pcEnd_, V, 0, outLoc);
    else
        *outLoc = OperandLocation::StackTop();
}


void
Interpreter::readUnaryOperandValues(Opcode op, Opcode baseOp,
                                    MutHandle<Value> in,
                                    OperandLocation *outLoc,
                                    int32_t *opBytes)
{
    OperandLocation inLoc;
    readUnaryOperandLocations(op, baseOp, &inLoc, outLoc, opBytes);

    // Read the operands.  Read the rhs first because if both lhs and
    // rhs are read from the StackTop, then they should be popped in
    // the right order (rhs, then lhs).
    in = readOperand(inLoc);
}


} // namespace Interp
} // namespace Whisper

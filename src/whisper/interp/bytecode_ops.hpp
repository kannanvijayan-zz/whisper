#ifndef WHISPER__INTERP__BYTECODE_OPS_HPP
#define WHISPER__INTERP__BYTECODE_OPS_HPP

#include "common.hpp"
#include "interp/bytecode_defn.hpp"

namespace Whisper {
namespace Interp {


/**
 * Whisper uses bytecode interpreter that operates on a hybrid stack
 * model.  The "hybrid" aspect occurs because the available opcodes
 * include both stack-like and register-like variants for many
 * common ops.
 *
 * The logical layout of an interpreter frame's execution context
 * includes the following "register" spaces:
 *      Constants - an addressable constant pool.
 *      Arguments - the actual arguments list.
 *      Locals    - the locals list.
 *      Stack     - the operand stack.
 *
 * Some op variants directly procure their inputs from these memory
 * areas, while others ops act as regular stack operations.
 *
 * Instruction Encoding
 * --------------------
 * Instructions are encoded with the first byte being an opcode, and
 * subsequent bytes encoing any operands the opcode might take.
 *
 * The format of the subsequent bytes can be classified into one of
 * several kinds.  Each kind is given both a short and a long name.
 *
 *      Empty (E)
 *
 *          Has zero subsequent bytes.
 *
 *      Integer<N> (I<N>)
 *
 *          There are 4 variants of this kind, for N=1, N=2, N=3, and N=4,
 *          encoding signed integers of varying widths.
 *
 *          The subsequent integers are read in little-endian format.
 *          The high bit of the last value is sign extended.
 *
 *      Unsigned<N> (U<N>)
 *
 *          There are 4 variants of this kind, for N=1, N=2, N=3, and N=4,
 *          encoding unsigned integers of varying widths.
 *
 *          The subsequent integers are read in little-endian format.
 *
 *      Value (V, VV, VVV, etc..)
 *
 *          One or more input operands are provided, each which may come
 *          from one of the virtual register files, or be an immediate.
 *
 *          Each operand area's first byte is encoded:
 *
 *              VVVV-RRRR
 *
 *              VVVV forms the beginning of either the register index
 *              or immediate value to be provided.
 *
 *              RRRR specifies the size class and interpretation of the
 *              index.
 *                  if 0000 - Constant register file, where VVVV specifies
 *                      the register number.
 *                  if 0001 - Constant register file, where VVVV and the
 *                      following byte specifies the register number.
 *                  if 0010 - Constant register file, where VVVV and the
 *                      following three bytes specify the register number.
 *
 *                  if 0100 - Argument register file, where VVVV specifies
 *                      the register number.
 *                  if 0101 - Argument register file, where VVVV and the
 *                      following byte specifies the register number.
 *                  if 0110 - Argument register file, where VVVV and the
 *                      following three bytes specify the register number.
 *
 *                  if 1000 - Local register file, where VVVV specifies
 *                      the register number.
 *                  if 1001 - Local register file, where VVVV and the
 *                      following byte specifies the register number.
 *                  if 1010 - Local register file, where VVVV and the
 *                      following three bytes specify the register number.
 *
 *                  if 1100 - Stack register file, where VVVV specifies
 *                      the register number.
 *                  if 1101 - Stack register file, where VVVV and the
 *                      following byte specifies the register number.
 *                  if 1110 - Stack register file, where VVVV and the
 *                      following three bytes specify the register number.
 *
 *                  if 0011 - Immediate unsigned value specified by VVVV.
 *                  if 0111 - Immediate signed value specified by VVVV and
 *                      following byte.
 *                  if 1011 - Immediate signed value specified by VVVV and
 *                      following two bytes.
 *                  if 1111 - Immediate signed value specified by VVVV and
 *                      following three bytes.
 *
 *          Note that the above encoding allows for a maximum of 0xFFFFFFF,
 *          or roughly 256 million registers in each of the respective files.
 */

enum class OpcodeFormat : uint8_t
{
    E               = 0x00,
    I1, I2, I3, I4,
    U1, U2, U3, U4,
    V, VV, VVV
};


//
// Enumeration of all the spaces an operand can reference.
// |StackTop| is the logical operand space representing the top of
// the stack.  This is a useful concept to describe instructions
// which read multiple values from the top of the stack and write back
// to the top of the stack.
//
enum class OperandSpace : uint8_t
{
    Constant    = 0,
    Argument    = 1,
    Local       = 2,
    Stack       = 3,
    Immediate,
    StackTop,
    LIMIT
};

const char *OperandSpaceString(OperandSpace space);

constexpr unsigned OperandSignificantBits = 28;
constexpr uint32_t OperandMaxIndex = 0x0fffffffUL;
constexpr int32_t OperandMaxUnsignedValue = 0x0fffffffUL;
constexpr int32_t OperandMaxSignedValue = 0x07ffffffL;
constexpr int32_t OperandMinSignedValue = -OperandMaxSignedValue - 1;

//
// OperandLocation is a helper class used mostly for code generation.
// It encapsulates the notion of the specific location of an operand.
//
class OperandLocation
{
  private:
    OperandSpace space_;
    uint32_t indexOrValue_;

    OperandLocation(OperandSpace space, uint32_t indexOrValue);

    static constexpr uint32_t IsSignedBit = 0x10000000;

  public:
    OperandLocation();

    static OperandLocation Constant(uint32_t index);
    static OperandLocation Argument(uint32_t index);
    static OperandLocation Local(uint32_t index);
    static OperandLocation Stack(uint32_t index);
    static OperandLocation Immediate(uint32_t value);
    static OperandLocation Immediate(int32_t value);
    static OperandLocation StackTop();

    OperandSpace space() const;

    bool isValid() const;

    bool isConstant() const;
    bool isArgument() const;
    bool isLocal() const;
    bool isStack() const;
    bool isImmediate() const;
    bool isStackTop() const;

    bool isUnsigned() const;
    bool isSigned() const;

    uint32_t index() const;

    uint32_t unsignedValue() const;
    int32_t signedValue() const;

    bool isWritable() const;
};

//
// Flags describing opcodes.
//
enum OpcodeFlags : uint32_t
{
    OPF_None                = 0x0,
    OPF_SectionPrefix       = 0x1,
    OPF_Control             = 0x2
};


enum class Opcode : uint16_t
{
    INVALID = 0,
#define OP_ENUM_(name, ...) name,
    WHISPER_BYTECODE_SEC0_OPS(OP_ENUM_)
#undef OP_ENUM_
    LIMIT
};

void InitializeOpcodeInfo();

uint32_t ReadOpcode(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
                    Opcode *opcode);
bool IsValidOpcode(Opcode op);

const char *GetOpcodeName(Opcode opcode);
OpcodeFormat GetOpcodeFormat(Opcode opcode);
int8_t GetOpcodeSection(Opcode opcode);
OpcodeFlags GetOpcodeFlags(Opcode opcode);
uint8_t GetOpcodeEncoding(Opcode opcode);

uint8_t GetOpcodePopped(Opcode opcode);
uint8_t GetOpcodePushed(Opcode opcode);

uint8_t GetOpcodeOperandCount(OpcodeFormat fmt);
uint32_t ReadOperandLocation(const uint8_t *bytecodeData,
                             const uint8_t *bytecodeEnd,
                             OpcodeFormat fmt, uint8_t operandNo,
                             OperandLocation *location);
    

} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODE_OPS_HPP

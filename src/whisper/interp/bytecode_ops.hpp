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
 * includes the following memory spaces:
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
 *      Immediate<N> (I<N>)
 *
 *          There are 4 variants of this kind, for N=1, N=2, N=3, and N=4,
 *          encoding integers of varying widths.
 *
 *          The subsequent integers are read in little-endian format.
 *
 *      Value (V)
 *
 *          Exactly one input operand is provided, which may come from one
 *          of the virtual register files, or be an immediate.
 *
 *          The operand area's first byte is encoded:
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
 *                  if 0011 - Immediate value specified by VVVV.
 *                  if 0111 - Immediate value specified by VVVV and following
 *                      byte.
 *                  if 1011 - Immediate value specified by VVVV and following
 *                      two bytes.
 *                  if 1111 - Immediate value specified by VVVV and following
 *                      three bytes.
 *
 */

enum class OperandFormat : uint8_t
{
    E               = 0x00,
    I1              = 0x01,
    I2              = 0x02,
    I3              = 0x03,
    I4              = 0x04,
    V               = 0x05,
    VV              = 0x06,
    VVV             = 0x07
};

enum class OperandSpace : uint8_t
{
    Constant    = 0,
    Argument    = 1,
    Local       = 2,
    Stack       = 3,
    Immediate,
    StackTop
};

constexpr unsigned OperandSignificantBits = 28;
constexpr uint32_t OperandMaxIndex = 0x0fffffffUL;
constexpr int32_t OperandMaxUnsignedValue = 0x0fffffffUL;
constexpr int32_t OperandMaxSignedValue = 0x07ffffffL;
constexpr int32_t OperandMinSignedValue = -OperandMaxSignedValue - 1;

class OperandLocation
{
  private:
    OperandSpace space_;
    uint32_t indexOrValue_;

    OperandLocation(OperandSpace space, uint32_t indexOrValue);

    static constexpr uint32_t IsSignedBit = 0x10000000;

  public:
    static OperandLocation Constant(uint32_t index);
    static OperandLocation Argument(uint32_t index);
    static OperandLocation Local(uint32_t index);
    static OperandLocation Stack(uint32_t index);
    static OperandLocation Immediate(uint32_t value);
    static OperandLocation Immediate(int32_t value);
    static OperandLocation StackTop();

    OperandSpace space() const;

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

bool IsValidOpcode(Opcode op);

const char *GetOpcodeName(Opcode opcode);
OperandFormat GetOperandFormat(Opcode opcode);
int8_t GetOpcodeSection(Opcode opcode);
OpcodeFlags GetOpcodeFlags(Opcode opcode);
uint8_t GetOpcodeEncoding(Opcode opcode);

    

} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODE_OPS_HPP

#ifndef WHISPER__INTERP__BYTECODE_DEFN_HPP
#define WHISPER__INTERP__BYTECODE_DEFN_HPP


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
 *      Register (R)
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
    R               = 0x05,
    RR              = 0x06,
    RRR             = 0x07
};

enum OpFlags : uint32_t
{
    OPF_None        = 0x0,
    OPF_Control     = 0x1,
};


// Macro iterating over all heap types.
#define WHISPER_BYTECODE_OP_TYPES(_)                                \
/* Name         Format  Flags                                     */\
\
_(Nop,          E,      None                                       )\
\
_(Ret_S,        E,      Control                                    )\
_(Ret_R,        R,      Control                                    )\
\
_(If_S,         E,      Control                                    )\
_(If_R,         R,      Control                                    )\
_(IfNot_S,      E,      Control                                    )\
_(IfNot_R,      R,      Control                                    )\
\
_(Add_SSS,      E,      None                                       )\
_(Add_SSR,      R,      None                                       )\
_(Add_SRS,      R,      None                                       )\
_(Add_SRR,      RR,     None                                       )\
_(Add_RSS,      R,      None                                       )\
_(Add_RSR,      RR,     None                                       )\
_(Add_RRS,      RR,     None                                       )\
_(Add_RRR,      RRR     None                                       )\



} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__BYTECODE_DEFN_HPP

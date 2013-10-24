
#include "debug.hpp"
#include "interp/bytecode_ops.hpp"

namespace Whisper {
namespace Interp {

//
// OpcodeFormat
//

bool
IsValidOpcodeFormat(OpcodeFormat fmt)
{
    switch (fmt) {
      case OpcodeFormat::E:
      case OpcodeFormat::I1:
      case OpcodeFormat::I2:
      case OpcodeFormat::I3:
      case OpcodeFormat::I4:
      case OpcodeFormat::Ix:
      case OpcodeFormat::U1:
      case OpcodeFormat::U2:
      case OpcodeFormat::U3:
      case OpcodeFormat::U4:
      case OpcodeFormat::V:
      case OpcodeFormat::VV:
      case OpcodeFormat::VVV:
        return true;
    }
    return false;
}

uint32_t
OpcodeFormatNumber(OpcodeFormat fmt)
{
    WH_ASSERT(IsValidOpcodeFormat(fmt));
    return ToUInt32(fmt);
}


//
// OperandSpaceString
//

const char *
OperandSpaceString(OperandSpace space)
{
    switch (space) {
      case OperandSpace::Constant:  return "Constant";
      case OperandSpace::Argument:  return "Argument";
      case OperandSpace::Local:     return "Local";
      case OperandSpace::Stack:     return "Stack";
      case OperandSpace::Immediate: return "Immediate";
      case OperandSpace::StackTop:  return "StackTop";
      default:
        break;
    };
    return "INVALID";
}

//
// OperandLocation
//

OperandLocation::OperandLocation(OperandSpace space, uint32_t indexOrValue)
  : space_(space), indexOrValue_(indexOrValue)
{
}

OperandLocation::OperandLocation()
  : space_(OperandSpace::LIMIT), indexOrValue_(0)
{}

/*static*/ OperandLocation
OperandLocation::Constant(uint32_t index)
{
    WH_ASSERT(index <= OperandMaxIndex);
    return OperandLocation(OperandSpace::Constant, index);
}

/*static*/ OperandLocation
OperandLocation::Argument(uint32_t index)
{
    WH_ASSERT(index <= OperandMaxIndex);
    return OperandLocation(OperandSpace::Argument, index);
}

/*static*/ OperandLocation
OperandLocation::Local(uint32_t index)
{
    WH_ASSERT(index <= OperandMaxIndex);
    return OperandLocation(OperandSpace::Local, index);
}

/*static*/ OperandLocation
OperandLocation::Stack(uint32_t index)
{
    WH_ASSERT(index <= OperandMaxIndex);
    return OperandLocation(OperandSpace::Stack, index);
}

/*static*/ OperandLocation 
OperandLocation::Immediate(uint32_t value)
{
    WH_ASSERT(value <= OperandMaxUnsignedValue);
    return OperandLocation(OperandSpace::Immediate, value);
}

/*static*/ OperandLocation 
OperandLocation::Immediate(int32_t value)
{
    WH_ASSERT(value >= OperandMinSignedValue &&
              value <= OperandMaxSignedValue);
    uint32_t v = ToUInt32(value) & OperandMaxUnsignedValue;
    v |= IsSignedBit;
    return OperandLocation(OperandSpace::Immediate, v);
}

/*static*/ OperandLocation 
OperandLocation::StackTop()
{
    return OperandLocation(OperandSpace::StackTop, 0);
}

OperandSpace
OperandLocation::space() const
{
    return space_;
}

bool
OperandLocation::isValid() const
{
    return space_ < OperandSpace::LIMIT;
}

bool
OperandLocation::isConstant() const
{
    return space_ == OperandSpace::Constant;
}

bool
OperandLocation::isArgument() const
{
    return space_ == OperandSpace::Argument;
}

bool 
OperandLocation::isLocal() const
{
    return space_ == OperandSpace::Local;
}

bool 
OperandLocation::isStack() const
{
    return space_ == OperandSpace::Stack;
}

bool
OperandLocation::isImmediate() const
{
    return space_ == OperandSpace::Immediate;
}

bool
OperandLocation::isStackTop() const
{
    return space_ == OperandSpace::StackTop;
}

bool
OperandLocation::isUnsigned() const
{
    WH_ASSERT(isImmediate());
    return !isSigned();
}

bool
OperandLocation::isSigned() const
{
    WH_ASSERT(isImmediate());
    return indexOrValue_ & IsSignedBit;
}

uint32_t
OperandLocation::constantIndex() const
{
    WH_ASSERT(isConstant());
    return indexOrValue_;
}

uint32_t
OperandLocation::argumentIndex() const
{
    WH_ASSERT(isArgument());
    return indexOrValue_;
}

uint32_t
OperandLocation::localIndex() const
{
    WH_ASSERT(isLocal());
    return indexOrValue_;
}

uint32_t
OperandLocation::stackIndex() const
{
    WH_ASSERT(isStack());
    return indexOrValue_;
}

uint32_t
OperandLocation::anyIndex() const
{
    WH_ASSERT(isConstant() || isArgument() || isLocal() || isStack());
    return indexOrValue_;
}

uint32_t
OperandLocation::unsignedValue() const
{
    WH_ASSERT(isImmediate() && isUnsigned());
    return indexOrValue_ & OperandMaxUnsignedValue;
}

int32_t 
OperandLocation::signedValue() const
{
    WH_ASSERT(isImmediate() && isSigned());
    uint32_t uval = indexOrValue_ & OperandMaxUnsignedValue;
    if (uval > OperandMaxSignedValue)
        uval |= ToUInt32(0xF) << OperandSignificantBits;
    return ToInt32(uval);
}

bool
OperandLocation::isWritable() const
{
    return isArgument() || isLocal() || isStack() || isStackTop();
}


//
// OpcodeTraits
//

class OpcodeTraits
{
  private:
    const char *name_ = nullptr;
    Opcode opcode_ = Opcode::INVALID;
    OpcodeFormat format_ = OpcodeFormat::E;
    int8_t section_ = -1;
    OpcodeFlags flags_ = OPF_None;
    uint8_t popped_ = 0;
    uint8_t pushed_ = 0;
    uint8_t encoding_ = 0;

  public:
    inline OpcodeTraits() {}

    inline OpcodeTraits(const char *name, Opcode opcode, OpcodeFormat format,
                        int8_t section, OpcodeFlags flags, uint8_t popped,
                        uint8_t pushed, uint8_t encoding)
      : name_(name), opcode_(opcode), format_(format), section_(section),
        flags_(flags), popped_(popped), pushed_(pushed), encoding_(encoding)
    {}

    inline const char *name() const {
        return name_;
    }
    inline Opcode opcode() const {
        return opcode_;
    }
    inline OpcodeFormat format() const {
        return format_;
    }
    inline int8_t section() const {
        return section_;
    }
    inline OpcodeFlags flags() const {
        return flags_;
    }
    inline uint8_t pushed() const {
        return pushed_;
    }
    inline uint8_t popped() const {
        return popped_;
    }
    inline uint8_t encoding() const {
        return encoding_;
    }
};

enum class Opcode_Sec0 : uint8_t
{
    INVALID = 0,
#define OP_ENUM_(name, ...) name,
    WHISPER_BYTECODE_SEC0_OPS(OP_ENUM_)
#undef OP_ENUM_
    LIMIT
};

uint16_t
OpcodeNumber(Opcode opcode)
{
    return ToUInt16(opcode);
}

static bool OPCODE_TRAITS_INITIALIZED = false;
static OpcodeTraits OPCODE_TRAITS[static_cast<unsigned>(Opcode::LIMIT)];

void
InitializeOpcodeInfo()
{
    WH_ASSERT(!OPCODE_TRAITS_INITIALIZED);
    for (unsigned i = 0; i < static_cast<unsigned>(Opcode::LIMIT); i++) {
        OPCODE_TRAITS[i] = OpcodeTraits(nullptr, Opcode::INVALID,
                                        OpcodeFormat::E, -1, OPF_None,
                                        /*pushed=*/0, /*popped=*/0,
                                        /*encoding=*/0);
    }

#define INIT_(op, format, section, popped, pushed, flags) \
    OPCODE_TRAITS[static_cast<unsigned>(Opcode::op)] = \
        OpcodeTraits(#op, Opcode::op, OpcodeFormat::format, section, flags, \
                     popped, pushed, ToUInt8(Opcode_Sec0::op));
    WHISPER_BYTECODE_SEC0_OPS(INIT_)
#undef INIT_
    OPCODE_TRAITS_INITIALIZED = true;
}

uint32_t
ReadOpcode(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
           Opcode *opcode)
{
    WH_ASSERT_IF(bytecodeEnd, bytecodeData < bytecodeEnd);

    uint16_t opval = bytecodeData[0];
    uint32_t nread = 1;

    WH_ASSERT(opval != 0);

    if (opval <= WHISPER_BYTECODE_MAX_SECTION) {
        WH_ASSERT_IF(bytecodeEnd, (bytecodeData + 1) < bytecodeEnd);
        opval <<= 8;
        opval |= bytecodeData[1];
        nread++;
    }

    WH_ASSERT(IsValidOpcode(static_cast<Opcode>(opval)));
    if (opcode)
        *opcode = static_cast<Opcode>(opval);
    return nread;
}

bool
IsValidOpcode(Opcode op)
{
    WH_ASSERT(OPCODE_TRAITS_INITIALIZED);

    if (op == Opcode::INVALID || op >= Opcode::LIMIT)
        return false;

    // A valid opcode has a valid section (>= 0).
    return OPCODE_TRAITS[static_cast<unsigned>(op)].section() >= 0;
}

const char *
GetOpcodeName(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].name();
}

OpcodeFormat
GetOpcodeFormat(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].format();
}

int8_t
GetOpcodeSection(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].section();
}

OpcodeFlags
GetOpcodeFlags(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].flags();
}

uint8_t
GetOpcodeEncoding(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].encoding();
}

uint8_t
GetOpcodePopped(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].popped();
}

uint8_t
GetOpcodePushed(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].pushed();
}

uint8_t
GetOpcodeOperandCount(OpcodeFormat fmt)
{
    uint8_t result = 0;
    for (uint32_t val = ToUInt32(fmt); val > 0; val >>= 4)
        result++;
    return result;
}

template <unsigned BYTES, bool SIGNED>
static uint32_t
ReadImmediateOperand(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
                     OperandLocation *location)
{
    WH_ASSERT_IF(bytecodeEnd, (bytecodeData + BYTES) <= bytecodeEnd);
    uint32_t val = 0;
    uint8_t i = 0;
    for (; i < BYTES; i++)
        val |= ToUInt32(bytecodeData[i]) << (i*8);

    if (SIGNED && (bytecodeData[i-1] & 0x80)) {
        for (; i < 4; i++)
            val |= ToUInt32(0xFF) << (i*8);
    }

    if (SIGNED) {
        if (location)
            *location = OperandLocation::Immediate(ToInt32(val));
    } else {
        if (location)
            *location = OperandLocation::Immediate(val);
    }
    return BYTES;
}

template <bool SIGNED>
static uint32_t
ReadImmediateXOperand(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
                      OperandLocation *location)
{
    WH_ASSERT_IF(bytecodeEnd, bytecodeData < bytecodeEnd);

    uint32_t val = 0;
    uint8_t i = 0;
    // Read bytes while high bit is 1 (up to 4 bytes).
    for (;; i++) {
        WH_ASSERT_IF(bytecodeEnd, (bytecodeData + i) < bytecodeEnd);
        uint8_t b = bytecodeData[i];
        val |= ToUInt32(b & 0x7Fu) << (i*7);
        if ((b & 0x80u) == 0)
            break;
    }

    // Test last data bit of last byte to see if value is negative.
    if (SIGNED && (bytecodeData[i-1] & 0x40))
        val |= ToUInt32(-1) << (i*7);

    if (SIGNED) {
        if (location)
            *location = OperandLocation::Immediate(ToInt32(val));
    } else {
        if (location)
            *location = OperandLocation::Immediate(val);
    }
    return i;
}

static void
ReadValueOperandNumber(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
                       uint8_t bytes, uint32_t *result)
{
    WH_ASSERT(result);
    WH_ASSERT(bytes <= 3);
    WH_ASSERT_IF(bytecodeEnd, (bytecodeData + bytes) <= bytecodeEnd);

    // Keep value in result.
    if (bytes == 0)
        return;

    uint32_t val = *result;
    for (uint8_t i = 0; i < bytes; i++)
        val |= ToUInt32(bytecodeData[i]) << (4 + i*8);

    *result = val;
}

static uint32_t
ReadValueOperand(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
                 OperandLocation *location)
{
    WH_ASSERT_IF(bytecodeEnd, bytecodeData < bytecodeEnd);
    uint8_t firstByte = bytecodeData[0];

    // If low 2 bits are 0x3, then this is an encoded immediate.
    uint8_t idxBytes = firstByte & 0x3;
    OperandSpace opSpace;
    if (idxBytes == 0x3) {
        opSpace = OperandSpace::Immediate;
        idxBytes = (firstByte >> 2) & 0x3;
    } else {
        opSpace = static_cast<OperandSpace>((firstByte >> 2) & 0x3);
    }

    WH_ASSERT_IF(bytecodeEnd, (bytecodeData + 1 + idxBytes) <= bytecodeEnd);

    uint32_t val = firstByte >> 4;
    ReadValueOperandNumber(bytecodeData+1, bytecodeEnd, idxBytes, &val);

    // Sign extend the value if it is signed.
    if (opSpace == OperandSpace::Immediate) {
        uint32_t signBit = ToUInt32(1) << ((idxBytes * 8) + 3);
        if (val & signBit)
            val |= ~((signBit << 1) - 1);
    }

    // Generate OperandLocation value.
    switch (opSpace) {
      case OperandSpace::Constant:
        if (location)
            *location = OperandLocation::Constant(val);
        break;

      case OperandSpace::Argument:
        if (location)
            *location = OperandLocation::Argument(val);
        break;

      case OperandSpace::Local:
        if (location)
            *location = OperandLocation::Local(val);
        break;

      case OperandSpace::Stack:
        if (location)
            *location = OperandLocation::Stack(val);
        break;

      case OperandSpace::Immediate:
        if (location)
            *location = OperandLocation::Immediate(ToInt32(val));
        break;

      default:
        WH_UNREACHABLE("Read Invalid OperandSpace.");
    }

    return idxBytes + 1;
}

uint32_t
ReadOperandLocation(const uint8_t *bytecodeData, const uint8_t *bytecodeEnd,
                    OpcodeFormat fmt, uint8_t operandNo,
                    OperandLocation *location)
{
    WH_ASSERT(fmt != OpcodeFormat::E);
    WH_ASSERT(operandNo < GetOpcodeOperandCount(fmt));
    WH_ASSERT_IF(bytecodeEnd, bytecodeData < bytecodeEnd);

    // Find the right reduced operand.
    uint32_t fmtPart = OpcodeFormatNumber(fmt);
    fmtPart >>= operandNo * OpcodeFormatComponentBits;
    fmtPart &= OpcodeFormatMask;

    switch (static_cast<OpcodeFormat>(fmtPart)) {
      case OpcodeFormat::I1:
        return ReadImmediateOperand<1, true>(bytecodeData, bytecodeEnd,
                                             location);

      case OpcodeFormat::I2:
        return ReadImmediateOperand<2, true>(bytecodeData, bytecodeEnd,
                                             location);

      case OpcodeFormat::I3:
        return ReadImmediateOperand<3, true>(bytecodeData, bytecodeEnd,
                                             location);

      case OpcodeFormat::I4:
        return ReadImmediateOperand<4, true>(bytecodeData, bytecodeEnd,
                                             location);

      case OpcodeFormat::Ix:
        return ReadImmediateXOperand<true>(bytecodeData, bytecodeEnd,
                                           location);

      case OpcodeFormat::U1:
        return ReadImmediateOperand<1, false>(bytecodeData, bytecodeEnd,
                                              location);

      case OpcodeFormat::U2:
        return ReadImmediateOperand<2, false>(bytecodeData, bytecodeEnd,
                                              location);

      case OpcodeFormat::U3:
        return ReadImmediateOperand<3, false>(bytecodeData, bytecodeEnd,
                                              location);

      case OpcodeFormat::U4:
        return ReadImmediateOperand<4, false>(bytecodeData, bytecodeEnd,
                                              location);

      case OpcodeFormat::V:
        return ReadValueOperand(bytecodeData, bytecodeEnd, location);

      default:
        break;
    }
    WH_UNREACHABLE("Invalid operand format.");
    return 0;
}


} // namespace Interp
} // namespace Whisper

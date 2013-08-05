
#include "debug.hpp"
#include "interp/bytecode_ops.hpp"

namespace Whisper {
namespace Interp {

//
// OpernadLocation
//

OperandLocation::OperandLocation(OperandSpace space, uint32_t indexOrValue)
  : space_(space), indexOrValue_(indexOrValue)
{
}

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
    uint32_t v = static_cast<uint32_t>(value) & OperandMaxUnsignedValue;
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
OperandLocation::isConstant() const
{
    return space_ == OperandSpace::Constant;
}

bool
OperandLocation::isArgument() const
{
    return space_ == OperandSpace::Constant;
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
OperandLocation::index() const
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
        uval |= static_cast<uint32_t>(0xF) << OperandSignificantBits;
    return static_cast<int32_t>(uval);
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
    OperandFormat format_ = OperandFormat::E;
    int8_t section_ = -1;
    OpcodeFlags flags_ = OPF_None;
    uint8_t encoding_ = 0;

  public:
    inline OpcodeTraits() {}

    inline OpcodeTraits(const char *name, Opcode opcode, OperandFormat format,
                        int8_t section, OpcodeFlags flags, uint8_t encoding)
      : name_(name), opcode_(opcode), format_(format), section_(section),
        flags_(flags), encoding_(encoding)
    {}

    inline const char *name() const {
        return name_;
    }
    inline Opcode opcode() const {
        return opcode_;
    }
    inline OperandFormat format() const {
        return format_;
    }
    inline int8_t section() const {
        return section_;
    }
    inline OpcodeFlags flags() const {
        return flags_;
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

static bool OPCODE_TRAITS_INITIALIZED = false;
static OpcodeTraits OPCODE_TRAITS[static_cast<unsigned>(Opcode::LIMIT)];

void
InitializeOpcodeInfo()
{
    WH_ASSERT(!OPCODE_TRAITS_INITIALIZED);
    for (unsigned i = 0; i < static_cast<unsigned>(Opcode::LIMIT); i++) {
        OPCODE_TRAITS[i] = OpcodeTraits(nullptr, Opcode::INVALID,
                                        OperandFormat::E, -1, OPF_None, 0);
    }

#define INIT_(op, format, section, flags) \
    OPCODE_TRAITS[static_cast<unsigned>(Opcode::op)] = \
        OpcodeTraits(#op, Opcode::op, OperandFormat::format, section, flags, \
                     static_cast<uint8_t>(Opcode_Sec0::op));
    WHISPER_BYTECODE_SEC0_OPS(INIT_)
#undef INIT_
}

bool
IsValidOpcode(Opcode op)
{
    WH_ASSERT(OPCODE_TRAITS_INITIALIZED);

    // A valid opcode has a valid section (>= 0).
    return OPCODE_TRAITS[static_cast<unsigned>(op)].section() >= 0;
}

const char *
GetOpcodeName(Opcode opcode)
{
    WH_ASSERT(IsValidOpcode(opcode));
    return OPCODE_TRAITS[static_cast<unsigned>(opcode)].name();
}

OperandFormat
GetOperandFormat(Opcode opcode)
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


} // namespace Interp
} // namespace Whisper

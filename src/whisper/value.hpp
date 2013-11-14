#ifndef WHISPER__VALUE_HPP
#define WHISPER__VALUE_HPP

#include <limits>
#include "common.hpp"
#include "debug.hpp"

namespace Whisper {


//
// Value
//
// A value is a 64-bit integer value, which can represent pointers to
// objects, strings, doubles, as well as immediate values of primitive types.
//
// At least the low 3 bits of every value are used as a tag.
//
// The notable aspect of the value boxing format is its treatment of
// doubles.  A value cannot immediately represent all doubles, but it
// can represent a range of common double values as immediates.  Other
// double values must be heap allocated.
//
//    000 - Object
//    001 - String
//    010 - HeapDouble
//    011, 100 - ImmDouble (Low, High)
//
//    101
//      00000 - NaN
//      00001 - NegInf
//      00010 - PosInf
//      00011 - NegZero
//      00100 - Integer (up to 51 bits)
//
//    110
//      SSS00 - 8-bit immediate string (up to 7 chars).
//      ???01 - UNUSED
//      SS010 - 16-bit immediate string (up to 3 chars).
//      00110 - Index string (value is positive int32).
//      00011 - Undefined
//      01011 - Null
//      00111 - False
//      01111 - True
//      10011 - UNUSED
//      10111 - UNUSED
//      11111 - UNUSED
//
//    111 - UNUSED
//
//  PPPP-PPPP PPPP-PPPP ... PPPP-PPPP PPPP-PPPP PPPP-P000 - Object ptr.
//  PPPP-PPPP PPPP-PPPP ... PPPP-PPPP PPPP-PPPP PPPP-P001 - Heap string ptr.
//  PPPP-PPPP PPPP-PPPP ... PPPP-PPPP PPPP-PPPP PPPP-P010 - Heap double ptr
//  EEEE-EEEE MMMM-MMMM ... MMMM-MMMM MMMM-MMMM MMMM-S011 - ImmDoubleLo
//  EEEE-EEEE MMMM-MMMM ... MMMM-MMMM MMMM-MMMM MMMM-S100 - ImmDoubleHigh
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0000-0101 - NaN
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0000-1101 - NegInf
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0001-0101 - PosInf
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0001-1101 - NegZero
//  IIII-IIII IIII-IIII ... IIII-IIII IIII-IIII 0010-0101 - Int32
//  GGGG-GGGG FFFF-FFFF ... BBBB-BBBB AAAA-AAAA SSS0-0110 - ImmString8
//  CCCC-CCCC CCCC-CCCC ... AAAA-AAAA 0000-0000 SS01-0110 - ImmString16
//  IIII-IIII IIII-IIII ... IIII-IIII IIII-IIII 0011-0110 - ImmIndexString
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0001-1110 - Undefined
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0101-1110 - Null
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0011-1110 - False
//  0000-0000 0000-0000 ... 0000-0000 0000-0000 0111-1110 - True
//
//

namespace VM {
    class HeapThing;
    class HeapString;
    class HeapDouble;
    class Object;
}

// Logical value types.
enum class ValueType : uint8_t
{
    INVALID = 0,
    Object,
    Null,
    Undefined,
    Boolean,
    String,
    Number,
    LIMIT
};

// Representational value types.
enum class ValueRepr : uint8_t
{
    INVALID = 0,
    Object,
    HeapString,
    HeapDouble,
    ImmDoubleLow,
    ImmDoubleHigh,
    NaN,
    NegInf,
    PosInf,
    NegZero,
    Int32,
    ImmString8,
    ImmString16,
    ImmIndexString,
    Undefined,
    Null,
    False,
    True,
    LIMIT,
};

// Tag enumeration for values.
enum class ValueTag : uint8_t
{
    Object              = 0x00, // PPPPP-000

    HeapString          = 0x01, // PPPPP-001
    HeapDouble          = 0x02, // PPPPP-010
    ImmDoubleLow        = 0x03, // MMMMM-011
    ImmDoubleHigh       = 0x04, // MMMMM-100
    ExtNumber           = 0x05, // ?????-101 - Integer and special doubles
    StringAndRest       = 0x06  // ?????-110 - immstring, undef, null, bool
};

bool IsValidValueTag(ValueTag tag);
unsigned ValueTagNumber(ValueTag tag);

class Value
{
  public:
    static constexpr unsigned TagBits = 3;
    static constexpr uint64_t TagMask = (1u << TagBits) - 1;


    static constexpr uint64_t ExtNumberMask = 0xff;

    static constexpr unsigned NaNVal = 0x00 | 0x5;
    static constexpr unsigned NegInfVal = 0x80 | 0x5;
    static constexpr unsigned PosInfVal = 0x10 | 0x5;
    static constexpr unsigned NegZeroVal = 0x18 | 0x5;

    static constexpr unsigned Int32Shift = 8;
    static constexpr unsigned Int32Mask = 0xff;
    static constexpr unsigned Int32Code = 0x20 | 0x5;


    static constexpr unsigned ImmString8Mask = 0x1f;
    static constexpr unsigned ImmString8Code = 0x00 | 0x6;
    static constexpr unsigned ImmString8LengthMask = 0x07;
    static constexpr unsigned ImmString8LengthShift = 5;
    static constexpr unsigned ImmString8MaxLength = 7;
    static constexpr unsigned ImmString8DataShift = 8;

    static constexpr unsigned ImmString16Mask = 0x3f;
    static constexpr unsigned ImmString16Code = 0x10 | 0x6;
    static constexpr unsigned ImmString16LengthMask = 0x03;
    static constexpr unsigned ImmString16LengthShift = 6;
    static constexpr unsigned ImmString16MaxLength = 3;
    static constexpr unsigned ImmString16DataShift = 16;

    static constexpr unsigned ImmIndexStringMask = 0x3f;
    static constexpr unsigned ImmIndexStringCode = 0x30 | 0x6;
    static constexpr unsigned ImmIndexStringMaxLength = 10; // "2147483647"
    static constexpr unsigned ImmIndexStringDataShift = 8;

    static constexpr unsigned ImmStringMaxLength = ImmIndexStringMaxLength;

    static constexpr unsigned RestMask = 0xff;
    static constexpr unsigned UndefinedVal = 0x18 | 0x6;
    static constexpr unsigned NullVal = 0x58 | 0x6;

    static constexpr unsigned BoolMask = 0x3f;
    static constexpr unsigned BoolCode = 0x3e;
    static constexpr unsigned FalseVal = 0x38 | 0x6;
    static constexpr unsigned TrueVal = 0x78 | 0x6;

    // Invalid value is a null-pointer
    static constexpr uint64_t Invalid = 0u;

    // Check if a double value can be encoded as an immediate.
    static bool IsImmediateNumber(double dval);

    static int32_t ImmediateIndexValue(uint32_t length, const uint8_t *str);
    static int32_t ImmediateIndexValue(uint32_t length, const uint16_t *str);

    static bool IsImmediateIndexString(uint32_t length, const uint8_t *str);
    static bool IsImmediateIndexString(uint32_t length, const uint16_t *str);

  protected:
    uint64_t tagged_;

  public:
    Value();

  protected:
    // Raw uint64_t constructor is private.
    explicit Value(uint64_t tagged);

    ValueTag getTag() const;
    bool checkTag(ValueTag tag) const;

  public:

    uint64_t raw() const;

    //
    // Constructors.
    //
    static Value Undefined();
    static Value Int32(int32_t value);
    static Value Double(double dval);
    static Value Number(double dval);
    static Value HeapDouble(VM::HeapDouble *dbl);

    static Value NaN();
    static Value PosInf();
    static Value NegInf();
    static Value NegZero();

    static Value ImmString8(unsigned length, const uint8_t *data);
    static Value ImmString16(unsigned length, const uint16_t *data);
    static Value ImmIndexString(int32_t idx);
    static Value HeapString(VM::HeapString *str);

#if defined(ENABLE_DEBUG)

    bool isValid() const;

#endif // defined(ENABLE_DEBUG)

    //
    // Get type
    //

    ValueType type() const;

    //
    // Checker methods
    //

    bool isObject() const;
    bool isHeapString() const;
    bool isHeapDouble() const;

    bool isImmDoubleLow() const;
    bool isImmDoubleHigh() const;

    bool isNaN() const;
    bool isNegInf() const;
    bool isPosInf() const;
    bool isNegZero() const;

    bool isInt32() const;

    bool isImmString8() const;
    bool isImmString16() const;
    bool isImmIndexString() const;

    bool isUndefined() const;
    bool isNull() const;
    bool isFalse() const;
    bool isTrue() const;

    //
    // Storage class predicates.
    //

    bool isHeapThing() const;
    bool isPrimitive() const;
    bool isImmediate() const;

    bool isNumber() const;
    bool isImmString() const;
    bool isString() const;
    bool isBoolean() const;

    //
    // Value extraction.
    //

    VM::Object *objectPtr() const;
    VM::HeapString *heapStringPtr() const;
    VM::HeapDouble *heapDoublePtr() const;

    int32_t int32Value() const;
    double numberValue() const;

    unsigned immString8Length() const;
    uint8_t getImmString8Char(unsigned idx) const;

    template <typename CharT>
    inline uint32_t readImmString8(CharT *buf) const;

    unsigned immString16Length() const;
    uint16_t getImmString16Char(unsigned idx) const;

    template <typename CharT, bool Trunc=false>
    inline uint32_t readImmString16(CharT *buf) const;

    int32_t immIndexStringValue() const;
    unsigned immIndexStringLength() const;
    uint8_t getImmIndexStringChar(unsigned idx) const;

    template <typename CharT>
    inline uint32_t readImmIndexString(CharT *buf) const;

    unsigned immStringLength() const;
    uint16_t getImmStringChar(unsigned idx) const;

    template <typename CharT, bool Trunc=false>
    inline uint32_t readImmString(CharT *buf) const;
};


} // namespace Whisper

#endif // WHISPER__VALUE_HPP

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
// heap things, as well as immediate values of various types.
//
// The high 4 bits of a value are used for a type tag.
//
// The notable aspect of the value boxing format is its treatment of
// doubles.  A value cannot immediately represent all doubles, but it
// can represent a range of common double values as immediates.  Other
// double values must be heap allocated.
//
// Tag types:
//  Object          - pointer to object.
//  Null            - null value (low 60 bits are ignored).
//  Undef           - undefined value (low 60 bits are ignored).
//  Boolean         - boolean value (low bit holds value).
//  HeapString      - pointer to string on heap.
//  ImmString8      - immediate 8-bit character string.
//  ImmString16     - immediate 16-bit character string.
//  ImmDoubleLow    - immediate doubles (1.0 > value && -1.0 < value)
//  ImmDoubleHigh   - immediate doubles (1.0 <= value || -1.0 >= value)
//  SpecialImmDouble - immediate NaN, Inf, -Inf, and -0 (low 2 bits).
//  HeapDouble      - heap double.
//  Int32           - 32-bit integer.
//  Magic           - magic value.
//
// Object:
//  0000-00*W PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - Object pointer
//  0000-01*W PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - Foreign pointer
//
//      W is the weak bit.  If W=1, the reference is weak.
//
// Null & Undefined:
//  0000-10** ****-**** ****-**** ... ****-****     - Null value
//  0001-00** ****-**** ****-**** ... ****-****     - Undefined value
//
// Boolean:
//  0001-10** ****-**** ****-**** ... ****-****     - Boolean value
//
// ImmDoubleLow, ImmDoubleHigh, SpecialImmDouble, and HeapDouble:
//  001E-EEEE EEEM-MMMM MMMM-MMMM ... MMMM-MMMM     - PosImmDoubleSmall
//  010E-EEEE EEEM-MMMM MMMM-MMMM ... MMMM-MMMM     - PosImmDoubleBig
//  101E-EEEE EEEM-MMMM MMMM-MMMM ... MMMM-MMMM     - NegImmDoubleSmall
//  110E-EEEE EEEM-MMMM MMMM-MMMM ... MMMM-MMMM     - NegImmDoubleBig
//  0110-0*** ****-**** ****-**** ... ****-**XX     - SpecialImmDouble
//  1000-0W** PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - HeapDouble pointer
//
//      [Pos|Neg]ImmDouble[Small|Big] are "regular" double values which
//      are immediately representable.  The only requirement is that
//       they have an exponent value in the range [256, -255].
//
//      The doubles in the above range are represented directly, with no
//      boxing or unboxing required.
//
//      SpecialImmDouble can represent -0.0, NaN, Inf, and -Inf.  Each of these
//      options is enumerated in the low 2 bits (XX):
//          00 => -0.0
//          01 => NaN
//          10 => Inf
//          11 => -Inf
//
//      In a heap double reference, W is the weak bit.  If W=1, the
//      reference is weak.
//
// HeapString & ImmString8 & ImmString16:
//  0110-1W** PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - HeapString pointer
//  0111-0LLL AAAA-AAAA BBBB-BBBB ... GGGG-GGGG     - Immediate 8-bit string
//  0111-1*LL 0000-0000 AAAA-AAAA ... CCCC-CCCC     - Immediate 16-bit string
//
//      In a heap string reference, W is the weak bit.  If W=1, the
//      reference is weak.
//
//      Immediate strings come in two variants.  The first variant can
//      represent all strings of length up to 7 containing 8-bit chars
//      only.  The second variant can represent all strings of length up
//      to 3 containing 16-bit chars.
//
//      Characters are stored from high to low.
//
//      This representation allows a lexical comparison of 8-bit immediate
//      strings with other 8-bit immediate strings, and of 16-bit immediate
//      strings with other 16-bit immediate strings, by simply rotating
//      the value left by 8 bits and doing an integer compare.
//
// UNUSED:
//  1000-1*** ****-**** ****-**** ... ****-****
//  1001-0*** ****-**** ****-**** ... ****-****
//  1001-1*** ****-**** ****-**** ... ****-****
//
// Int32:
//  1110-0*** ****-**** ****-**** ... IIII-IIII     - Int32 value.
//
//      Only the low 32 bits are used for integer values.
//
// Magic:
//  1110-1*** MMMM-MMMM MMMM-MMMM ... MMMM-MMMM     - Magic value.
//
//      Magic values are used in implementation objects to store
//      arbitrary packed info, such as a combination of bitfields and
//      flags.
//
// UNUSED:
//  1111-**** ****-**** ****-**** ... ****-****
//
//

namespace VM {
    class HeapThing;
    class HeapString;
    class HeapDouble;
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

// Tag enumeration for values.
enum class ValueTag : uint8_t
{
    Object              = 0x00, // 0000-0

    Null                = 0x01, // 0000-1
    Undefined           = 0x02, // 0001-0
    Boolean             = 0x03, // 0001-1

    PosImmDoubleBig     = 0x04, // 0010-0 to 0011-1 (0x04 to 0x07)
    PosImmDoubleBig_2   = 0x05,
    PosImmDoubleBig_3   = 0x06,
    PosImmDoubleBig_4   = 0x07,
    PosImmDoubleSmall   = 0x08, // 0100-0 to 0101-1 (0x08 to 0x0B)
    PosImmDoubleSmall_2 = 0x09,
    PosImmDoubleSmall_3 = 0x0A,
    PosImmDoubleSmall_4 = 0x0B,
    NegImmDoubleBig     = 0x14, // 1010-0 to 1011-1 (0x14 to 0x17)
    NegImmDoubleBig_1   = 0x15,
    NegImmDoubleBig_2   = 0x16,
    NegImmDoubleBig_3   = 0x17,
    NegImmDoubleSmall   = 0x18, // 1100-0 to 1101-1 (0x18 to 0x1B)
    NegImmDoubleSmall_2 = 0x19,
    NegImmDoubleSmall_3 = 0x1A,
    NegImmDoubleSmall_4 = 0x1B,
    SpecialImmDouble    = 0x0C, // 0110-0
    HeapDouble          = 0x10, // 1000-0

    HeapString          = 0x0D, // 0110-1
    ImmString8          = 0x0E, // 0111-0
    ImmString16         = 0x0F, // 0111-1

    UNUSED_11           = 0x11, // 1000-1
    UNUSED_12           = 0x12, // 1001-0
    UNUSED_13           = 0x13, // 1001-1

    Int32               = 0x1C, // 1110-0
    Magic               = 0x1D, // 1110-1

    UNUSED_1E           = 0x1E, // 1111-0
    UNUSED_1F           = 0x1F  // 1111-1
};

inline uint8_t constexpr ValueTagNumber(ValueTag type) {
    return static_cast<uint8_t>(type);
}

inline bool IsValidValueTag(ValueTag tag) {
    return tag == ValueTag::Object              ||
           tag == ValueTag::Null                ||
           tag == ValueTag::Undefined           ||
           tag == ValueTag::Boolean             ||
           tag == ValueTag::PosImmDoubleBig     ||
           tag == ValueTag::PosImmDoubleSmall   ||
           tag == ValueTag::NegImmDoubleBig     ||
           tag == ValueTag::NegImmDoubleSmall   ||
           tag == ValueTag::SpecialImmDouble    ||
           tag == ValueTag::HeapDouble          ||
           tag == ValueTag::HeapString          ||
           tag == ValueTag::ImmString8          ||
           tag == ValueTag::ImmString16         ||
           tag == ValueTag::Int32               ||
           tag == ValueTag::Magic;
}

class Value
{
  public:
    // Constants relating to regular (non-double) tag bits.
    static constexpr unsigned RegularTagBits = 5;
    static constexpr unsigned RegularTagShift = 59;
    static constexpr uint64_t RegularTagMaskLow = 0x1Fu;
    static constexpr uint64_t RegularTagMaskHigh =
        RegularTagMaskLow << RegularTagShift;

    // The weak bit is the same bit across all pointer-type values.
    static constexpr unsigned WeakBit = 58;
    static constexpr uint64_t WeakMask = UInt64(1) << WeakBit;

    // High 6 bits of pointer values describe pointer type (native or foreign)
    static constexpr unsigned PtrTypeBits = 6;
    static constexpr unsigned PtrTypeShift = 58;
    static constexpr uint64_t PtrTypeMaskLow = 0x3Fu;
    static constexpr unsigned PtrType_Native = 0x0u;
    static constexpr unsigned PtrType_Foreign = 0x1u;

    static constexpr unsigned PtrValueBits = 56;
    static constexpr uint64_t PtrValueMask = (UInt64(1) << PtrValueBits) - 1;

    // Constants for double tag bits
    static constexpr unsigned DoubleTagBits = 3;
    static constexpr unsigned DoubleTagShift = 61;
    static constexpr uint64_t DoubleTagMaskLow = 0x7u;
    static constexpr uint64_t DoubleTagMaskHigh =
        DoubleTagMaskLow << DoubleTagShift;

    static constexpr unsigned DoubleTag_PosSmall = 0x1;     // 001*-**** ...
    static constexpr unsigned DoubleTag_PosBig = 0x2;       // 010*-**** ...
    static constexpr unsigned DoubleTag_NegSmall = 0x5;     // 101*-**** ...
    static constexpr unsigned DoubleTag_NegBig = 0x6;       // 110*-**** ...

    // Immediate values for special doubles.
    static constexpr uint64_t SpecialImmDoubleValueMask = 0x3;
    static constexpr unsigned NegZeroVal = 0x0;
    static constexpr unsigned NaNVal     = 0x1;
    static constexpr unsigned PosInfVal  = 0x2;
    static constexpr unsigned NegInfVal  = 0x3;

    // Constants relating to string bits.
    static constexpr unsigned ImmString8MaxLength = 7;
    static constexpr unsigned ImmString8LengthShift = 56;
    static constexpr uint64_t ImmString8LengthMaskLow = 0x7;
    static constexpr uint64_t ImmString8LengthMaskHigh =
        ImmString8LengthMaskLow << ImmString8LengthShift;

    static constexpr unsigned ImmString16MaxLength = 3;
    static constexpr unsigned ImmString16LengthShift = 56;
    static constexpr uint64_t ImmString16LengthMaskLow = 0x3;
    static constexpr uint64_t ImmString16LengthMaskHigh =
        ImmString16LengthMaskLow << ImmString16LengthShift;

    // Invalid value is a null-pointer
    static constexpr uint64_t Invalid = 0u;

  protected:
    uint64_t tagged_;

  public:
    Value();

  protected:
    // Raw uint64_t constructor is private.
    explicit Value(uint64_t tagged);

    ValueTag getTag() const;
    bool checkTag(ValueTag tag) const;

    template <typename T=void>
    inline T *getPtr() const;

    template <typename T>
    static inline Value MakePtr(ValueTag tag, T *ptr, bool weak=false);

    static Value MakeTag(ValueTag tag);
    static Value MakeTagValue(ValueTag tag, uint64_t ival);

  public:

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

    bool isNativeObject() const;

    template <typename T>
    inline bool isNativeObjectOf() const;

    bool isForeignObject() const;

    bool isNull() const;

    bool isUndefined() const;

    bool isBoolean() const;

    bool isHeapString() const;

    bool isImmString8() const;

    bool isImmString16() const;

    bool isPosImmDoubleSmall() const;
    bool isPosImmDoubleBig() const;
    bool isNegImmDoubleSmall() const;
    bool isNegImmDoubleBig() const;

    bool isImmDouble() const;

    bool isNegZero() const;

    bool isNaN() const;

    bool isPosInf() const;

    bool isNegInf() const;

    bool isHeapDouble() const;

    bool isInt32() const;

    bool isMagic() const;

    // Helper functions to check combined types.

    bool isString() const;

    bool isImmString() const;

    bool isNumber() const;

    bool isDouble() const;

    bool isSpecialImmDouble() const;

    bool isRegularImmDouble() const;

    bool isWeakPointer() const;

    //
    // Getter methods
    //
    template <typename T>
    inline T *getNativeObject() const;

    VM::HeapThing *getAnyNativeObject() const;

    template <typename T>
    inline T *getForeignObject() const;

    bool getBoolean() const;

    VM::HeapString *getHeapString() const;

    unsigned immString8Length() const;

    uint8_t getImmString8Char(unsigned idx) const;

    template <typename CharT>
    inline uint32_t readImmString8(CharT *buf) const;

    unsigned immString16Length() const;

    uint16_t getImmString16Char(unsigned idx) const;

    template <typename CharT>
    inline uint32_t readImmString16(CharT *buf) const;

    unsigned immStringLength() const;

    uint16_t getImmStringChar(unsigned idx) const;

    template <typename CharT>
    inline unsigned readImmString(CharT *buf) const;

    double getRegularImmDoubleValue() const;

    double getSpecialImmDoubleValue() const;

    double getImmDoubleValue() const;

    VM::HeapDouble *getHeapDouble() const;

    uint64_t getMagicInt() const;

    int32_t getInt32() const;

    //
    // Friend functions
    //
    friend Value NativeObjectValue(VM::HeapThing *obj);
    friend Value WeakNativeObjectValue(VM::HeapThing *obj);
    friend Value ForeignObjectValue(void *obj);
    friend Value WeakForeignObjectValue(void *obj);
    friend Value NullValue();
    friend Value UndefinedValue();
    friend Value BooleanValue(bool b);
    friend Value StringValue(VM::HeapString *str);
    template <typename CharT>
    friend Value String8Value(unsigned length, const CharT *data);
    template <typename CharT>
    friend Value String16Value(unsigned length, const CharT *data);
    template <typename CharT>
    friend Value StringValue(unsigned length, const CharT *data);
    friend Value MagicValue(uint64_t val);
    friend Value IntegerValue(int32_t i);
    friend Value DoubleValue(VM::HeapDouble *d);
    friend Value NaNValue();
    friend Value NegZeroValue();
    friend Value PosInfValue();
    friend Value NegInfValue();
    friend Value DoubleValue(double d);
};


Value NativeObjectValue(VM::HeapThing *obj);

Value WeakNativeObjectValue(VM::HeapThing *obj);

Value ForeignObjectValue(void *obj);

Value WeakForeignObjectValue(void *obj);

Value NullValue();

Value UndefinedValue();

Value BooleanValue(bool b);

Value StringValue(VM::HeapString *str);

template <typename CharT>
inline Value String8Value(unsigned length, const CharT *data);

template <typename CharT>
inline Value String16Value(unsigned length, const CharT *data);

template <typename CharT>
inline Value StringValue(unsigned length, const CharT *data);

Value DoubleValue(VM::HeapDouble *d);

Value NegZeroValue();

Value NaNValue();

Value PosInfValue();

Value NegInfValue();

Value DoubleValue(double dval);

Value MagicValue(uint64_t val);

Value IntegerValue(int32_t ival);


} // namespace Whisper

#endif // WHISPER__VALUE_HPP

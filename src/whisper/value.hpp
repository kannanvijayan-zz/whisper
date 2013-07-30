#ifndef WHISPER__VALUE_HPP
#define WHISPER__VALUE_HPP

#include <limits>
#include <type_traits>
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
//  ImmDoubleX      - immediate NaN, Inf, -Inf, and -0 (low 2 bits hold value).
//  HeapDouble      - heap double.
//  Int32           - 32-bit integer.
//  Magic           - magic value.
//
// Object:
//  0000-W000 PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - Object pointer
//  0000-W001 PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - Foreign pointer
//
//      W is the weak bit.  If W=1, the reference is weak.
//
// Null & Undefined:
//  0001-0000 0000-0000 0000-0000 ... 0000-0000     - Null value
//  0010-0000 0000-0000 0000-0000 ... 0000-0000     - Undefined value
//
// Boolean:
//  0011-0000 0000-0000 0000-0000 ... 0000-000V     - Boolean value
//
// HeapString & ImmString8 & ImmString16:
//  0100-W000 PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - HeapString pointer
//  0101-0LLL AAAA-AAAA BBBB-BBBB ... GGGG-GGGG     - Immediate 8-bit string
//  0110-0000 0000-0000 AAAA-AAAA ... CCCC-CCCC     - Immediate 16-bit string
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
// ImmDoubleLow, ImmDoubleHigh, ImmDoubleX, and HeapDouble:
//  0111-EEEE EEEM-MMMM MMMM-MMMM ... MMMM-MMMS     - ImmDoubleLow
//  1000-EEEE EEEM-MMMM MMMM-MMMM ... MMMM-MMMS     - ImmDoubleHigh
//  1001-0000 0000-0000 0000-0000 ... 0000-00XX     - ImmDoubleX
//  1010-W000 PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - HeapDouble pointer
//
//      ImmDoubleLo and ImmDoubleHi are "regular" double values which
//      are immediately representable.  The only requirement is that
//       they have an exponent value in the range [128, -127].
//
//      The boxed representation is achieved by rotating the native double
//      representation left by 1 bit.  Given the exponent requirement,
//      this naturally ensures that the top 4 bits of the boxed representation
//      are either 0x7 or 0x8.
//
//      ImmDoubleX can represent -0.0, NaN, Inf, and -Inf.  Each of these
//      options is enumerated in the low 2 bits (XX):
//          00 => -0.0, 
//          01 => NaN
//          10 => Inf
//          11 => -Inf
//
//      In a heap double reference, W is the weak bit.  If W=1, the
//      reference is weak.
//
// Int32:
//  1100-0000 0000-0000 0000-0000 ... IIII-IIII     - Int32 value.
//
//      Only the low 32 bits are used for integer values.
//
// Magic:
//  1101-MMMM MMMM-MMMM MMMM-MMMM ... MMMM-MMMM     - Magic value.
//
//      The low 60 bits can be used to store any required magic value
//      info.
//
// UNUSED:
//  1110-**** ****-**** ****-**** ... ****-****
//  1111-**** ****-**** ****-**** ... ****-****
//
//

namespace VM {
    class UntypedHeapThing;
    class Object;
    class HeapString;
    class HeapDouble;
}

// Type tag enumeration for values.
enum class ValueTag : uint8_t
{
    Object          = 0x0,

    Null            = 0x1,
    Undefined       = 0x2,
    Boolean         = 0x3,

    HeapString      = 0x4,
    ImmString8      = 0x5,
    ImmString16     = 0x6,

    ImmDoubleLow    = 0x7,
    ImmDoubleHigh   = 0x8,
    ImmDoubleX      = 0x9,
    HeapDouble      = 0xA,

    UNUSED_B        = 0xB,

    Int32           = 0xC,
    Magic           = 0xD,

    UNUSED_E        = 0xE,
    UNUSED_F        = 0xF
};

inline uint8_t constexpr ValueTagNumber(ValueTag type) {
    return static_cast<uint8_t>(type);
}

inline bool IsValidValueTag(ValueTag tag) {
    return tag == ValueTag::Object        || tag == ValueTag::Null         ||
           tag == ValueTag::Undefined     || tag == ValueTag::Boolean      ||
           tag == ValueTag::HeapString    || tag == ValueTag::ImmString8   ||
           tag == ValueTag::ImmString16   || tag == ValueTag::ImmDoubleLow ||
           tag == ValueTag::ImmDoubleHigh || tag == ValueTag::ImmDoubleX   ||
           tag == ValueTag::HeapDouble    || tag == ValueTag::Int32        ||
           tag == ValueTag::Magic;
}


enum class Magic : uint32_t
{
    INVALID         = 0,
    LIMIT
};

class Value
{
  public:
    // Constants relating to tag bits.
    static constexpr unsigned TagBits = 4;
    static constexpr unsigned TagShift = 60;
    static constexpr uint64_t TagMaskLow = 0xFu;
    static constexpr uint64_t TagMaskHigh = TagMaskLow << TagShift;

    // High 16 bits of pointer values do not contain address bits.
    static constexpr unsigned PtrHighBits = 8;
    static constexpr unsigned PtrTypeShift = 56;
    static constexpr unsigned PtrTypeMask = 0xFu;
    static constexpr unsigned PtrType_Native = 0x0u;
    static constexpr unsigned PtrType_Foreign = 0x1u;

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

    // The weak bit is the same bit across all pointer-type values.
    static constexpr unsigned WeakBit = 59;
    static constexpr uint64_t WeakMask = UInt64(1) << WeakBit;

    // Bounds for representable simple ImmDouble:
    // In order of: [PosMax, PosMin, NegMax, NegMin]
    //           SEEE-EEEE EEEE-MMMM MMMM-MMMM .... MMMM-MMMM
    // PosMax: 0100-0111 1111-1111 1111-1111 .... 1111-1111
    // PosMin: 0011-1000 0000-0000 0000-0000 .... 0000-0000
    // NegMax: 1011-1000 0000-0000 0000-0000 .... 0000-0000
    // MegMin: 1100-0111 1111-1111 1111-1111 .... 1111-1111
    template <bool NEG, unsigned EXP, bool MANT>
    struct GenerateDouble_ {
        static constexpr uint64_t Value =
            (UInt64(NEG) << 63) |
            (UInt64(EXP) << 52) |
            (MANT ? ((UInt64(MANT) << 52) - 1u) : 0);
    };
    static constexpr uint64_t ImmDoublePosMax =
        GenerateDouble_<false, 0x47f, true>::Value;
    static constexpr uint64_t ImmDoublePosMin =
        GenerateDouble_<false, 0x380, false>::Value;
    static constexpr uint64_t ImmDoubleNegMax =
        GenerateDouble_<true, 0x380, false>::Value;
    static constexpr uint64_t ImmDoubleNegMin =
        GenerateDouble_<true, 0x47f, true>::Value;

    // Immediate values for special doubles.
    static constexpr unsigned NegZeroVal = 0x0;
    static constexpr unsigned NaNVal     = 0x1;
    static constexpr unsigned PosInfVal  = 0x2;
    static constexpr unsigned NegInfVal  = 0x3;

    // Invalid value is a null-pointer
    static constexpr uint64_t Invalid = 0u;

  protected:
    uint64_t tagged_;

  public:
    Value();

  protected:
    // Raw uint64_t constructor is private.
    explicit Value(uint64_t tagged);

    bool checkTag(ValueTag tag) const;
    uint64_t removeTag() const;

    template <typename T=void>
    inline T *getPtr() const;

    template <typename T>
    static inline Value MakePtr(ValueTag tag, T *ptr);

    static Value MakeTag(ValueTag tag);
    static Value MakeTagValue(ValueTag tag, uint64_t ival);

  public:

#if defined(ENABLE_DEBUG)

    bool isValid() const;

#endif // defined(ENABLE_DEBUG)

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

    bool isImmDoubleLow() const;

    bool isImmDoubleHigh() const;

    bool isImmDoubleX() const;

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
    template <typename T=VM::Object>
    inline T *getNativeObject() const;

    VM::UntypedHeapThing *getAnyNativeObject() const;

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

    double getImmDoubleHiLoValue() const;

    double getImmDoubleXValue() const;

    double getImmDoubleValue() const;

    VM::HeapDouble *getHeapDouble() const;

    uint64_t getMagicInt() const;

    Magic getMagic() const;

    int32_t getInt32() const;

    //
    // Friend functions
    //
    template <typename T>
    friend Value NativeObjectValue(T *obj);
    template <typename T>
    friend Value WeakNativeObjectValue(T *obj);
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
    friend Value MagicValue(Magic m);
    friend Value IntegerValue(int32_t i);
    friend Value DoubleValue(VM::HeapDouble *d);
    friend Value NaNValue();
    friend Value NegZeroValue();
    friend Value PosInfValue();
    friend Value NegInfValue();
    friend Value DoubleValue(double d);
};


template <typename T>
inline Value NativeObjectValue(T *obj);

template <typename T>
inline Value WeakNativeObjectValue(T *obj);

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

Value MagicValue(Magic magic);

Value IntegerValue(int32_t ival);


} // namespace Whisper

#endif // WHISPER__VALUE_HPP

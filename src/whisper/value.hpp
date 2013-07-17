#ifndef WHISPER__VALUE_HPP
#define WHISPER__VALUE_HPP

#include "common.hpp"
#include "debug.hpp"

#include <limits>

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
//  0000-PPPP PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - Object pointer
//
// Null & Undefined:
//  0001-0000 0000-0000 0000-0000 ... 0000-0000     - Null value
//  0010-0000 0000-0000 0000-0000 ... 0000-0000     - Undefined value
//
// Boolean:
//  0011-0000 0000-0000 0000-0000 ... 0000-000V     - Boolean value
//
// HeapString & ImmString8 & ImmString16:
//  0100-PPPP PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - String pointer
//  0101-0LLL AAAA-AAAA BBBB-BBBB ... GGGG-GGGG     - Immediate 8-bit string
//  0110-0000 0000-0000 AAAA-AAAA ... CCCC-CCCC     - Immediate 16-bit string
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
//  1010-PPPP PPPP-PPPP PPPP-PPPP ... PPPP-PPPP     - Double pointer
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

class Object;
class String;
class Double;

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
            (static_cast<uint64_t>(NEG) << 63) |
            (static_cast<uint64_t>(EXP) << 52) |
            (MANT ? ((static_cast<uint64_t>(MANT) << 52) - 1u) : 0);
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

  private:
    uint64_t tagged_;

  public:
    inline Value() : tagged_(INVALID) {}

  private:
    // Raw uint64_t constructor is private.
    inline explicit Value(uint64_t tagged) : tagged_(tagged) {}

    inline bool checkTag(ValueTag tag) const {
        return (tagged_ >> TagShift) == ValueTagNumber(tag);
    }

    template <typename T=void>
    inline T *getPtr() const {
        WH_ASSERT(isObject() || isHeapString() || isHeapDouble());
        return reinterpret_cast<T *>(
                (static_cast<int64_t>(tagged_ << TagBits)) >> TagBits);
    }

    inline uint64_t removeTag() const {
        return (tagged_ << TagBits) >> TagBits;
    }

    template <typename T>
    inline static Value MakePtr(ValueTag tag, T *ptr) {
        WH_ASSERT(tag == ValueTag::Object || tag == ValueTag::HeapString ||
                  tag == ValueTag::HeapDouble);
        uint64_t ptrval = reinterpret_cast<uint64_t>(ptr);
        ptrval &= ~TagMaskHigh;
        ptrval |= static_cast<uint64_t>(tag) << TagShift;
        return Value(ptrval);
    }

    inline static Value MakeTag(ValueTag tag) {
        WH_ASSERT(IsValidValueTag(tag));
        return Value(static_cast<uint64_t>(tag) << TagShift);
    }

    inline static Value MakeTagValue(ValueTag tag, uint64_t ival) {
        WH_ASSERT(IsValidValueTag(tag));
        WH_ASSERT(ival <= (~static_cast<uint64_t>(0) >> TagBits));
        return Value(static_cast<uint64_t>(tag) << TagShift);
    }

#if defined(ENABLE_DEBUG)
    inline bool isValid() const {
        // If heap thing, pointer can't be zero.
        if (((tagged_ & 0x3) == 0) && ((tagged_ >> 4) == 0))
            return false;

        // Otherwise, check for unused tag range.
        unsigned tag = tagged_ & 0xFu;
        if (tag >= 0x8u && tag <= 0xBu)
            return false;

        return true;
    }
#endif // defined(ENABLE_DEBUG)

    //
    // Checker methods
    //

    inline bool isObject() const {
        return checkTag(ValueTag::Object);
    }

    inline bool isNull() const {
        return checkTag(ValueTag::Null);
    }

    inline bool isUndefined() const {
        return checkTag(ValueTag::Undefined);
    }

    inline bool isBoolean() const {
        return checkTag(ValueTag::Boolean);
    }

    inline bool isHeapString() const {
        return checkTag(ValueTag::HeapString);
    }

    inline bool isImmString8() const {
        return checkTag(ValueTag::ImmString8);
    }

    inline bool isImmString16() const {
        return checkTag(ValueTag::ImmString16);
    }

    inline bool isImmDoubleLow() const {
        return checkTag(ValueTag::ImmDoubleLow);
    }

    inline bool isImmDoubleHigh() const {
        return checkTag(ValueTag::ImmDoubleHigh);
    }

    inline bool isImmDoubleX() const {
        return checkTag(ValueTag::ImmDoubleX);
    }

    inline bool isNegZero() const {
        return isImmDoubleX() && (tagged_ & 0xFu) == NegZeroVal;
    }

    inline bool isNaN() const {
        return isImmDoubleX() && (tagged_ & 0xFu) == NaNVal;
    }

    inline bool isPosInf() const {
        return isImmDoubleX() && (tagged_ & 0xFu) == PosInfVal;
    }

    inline bool isNegInf() const {
        return isImmDoubleX() && (tagged_ & 0xFu) == NegInfVal;
    }

    inline bool isHeapDouble() const {
        return checkTag(ValueTag::HeapDouble);
    }

    inline bool isInt32() const {
        return checkTag(ValueTag::Int32);
    }

    inline bool isMagic() const {
        return checkTag(ValueTag::Magic);
    }

    // Helper functions to check combined types.

    inline bool isString() const {
        return isImmString8() || isImmString16() || isHeapString();
    }

    inline bool isImmString() const {
        return isImmString8() || isImmString16();
    }

    inline bool isNumber() const {
        return isImmDoubleLow() || isImmDoubleHigh() || isImmDoubleX() ||
               isHeapDouble()   || isInt32();
    }

    inline bool isDouble() const {
        return isImmDoubleLow() || isImmDoubleHigh() || isImmDoubleX() ||
               isHeapDouble();
    }

    inline bool isSpecialImmDouble() const {
       return isImmDoubleLow() || isImmDoubleHigh() || isImmDoubleX();
    }

    inline bool isRegularImmDouble() const {
       return isImmDoubleLow() || isImmDoubleHigh();
    }


    //
    // Getter methods
    //

    inline Object *getObject() const {
        WH_ASSERT(isObject());
        return getPtr<Object>();
    }

    inline bool getBoolean() const {
        WH_ASSERT(isBoolean());
        return tagged_ & 0x1;
    }

    inline String *getHeapString() const {
        WH_ASSERT(isHeapString());
        return getPtr<String>();
    }

    inline unsigned immString8Length() const {
        WH_ASSERT(isImmString8());
        return (tagged_ >> ImmString8LengthShift) & ImmString8LengthMaskLow;
    }

    inline uint8_t getImmString8Char(unsigned idx) const {
        WH_ASSERT(isImmString8());
        WH_ASSERT(idx < immString8Length());
        return (tagged_ >> (48 - (idx*8))) & 0xFFu;
    }

    template <typename CharT>
    inline unsigned readImmString8(CharT *buf) const {
        WH_ASSERT(isImmString8());
        unsigned length = immString8Length();
        for (unsigned i = 0; i < length; i++)
            buf[i] = (tagged_ >> (48 - (8 * i))) & 0xFFu;
        return length;
    }

    inline unsigned immString16Length() const {
        WH_ASSERT(isImmString16());
        return (tagged_ >> ImmString16LengthShift) & ImmString16LengthMaskLow;
    }

    inline uint16_t getImmString16Char(unsigned idx) const {
        WH_ASSERT(isImmString16());
        WH_ASSERT(idx < immString16Length());
        return (tagged_ >> (32 - (idx*16))) & 0xFFFFu;
    }

    template <typename CharT>
    inline unsigned readImmString16(CharT *buf) const {
        WH_ASSERT(isImmString16());
        unsigned length = immString16Length();
        for (unsigned i = 0; i < length; i++)
            buf[i] = (tagged_ >> (32 - (16 * i))) & 0xFFu;
        return length;
    }

    inline unsigned immStringLength() const {
        WH_ASSERT(isImmString());
        return isImmString8() ? immString8Length() : immString16Length();
    }

    inline uint16_t getImmStringChar(unsigned idx) const {
        WH_ASSERT(isImmString());
        return isImmString8() ? getImmString8Char(idx)
                              : getImmString16Char(idx);
    }

    template <typename CharT>
    inline unsigned readImmString(CharT *buf) const {
        WH_ASSERT(isImmString());
        return isImmString8() ? readImmString8<CharT>(buf)
                              : readImmString16<CharT>(buf);
    }

    inline double getImmDoubleHiLoValue() const {
        WH_ASSERT(isImmDoubleHigh() || isImmDoubleLow());
        return IntToDouble(RotateRight(tagged_, 1));
    }

    inline double getImmDoubleXValue() const {
        WH_ASSERT(isImmDoubleX());
        switch (tagged_) {
          case NaNVal:
            return std::numeric_limits<double>::quiet_NaN();
          case NegZeroVal:
            return -0.0;
          case PosInfVal:
            return std::numeric_limits<double>::infinity();
          case NegInfVal:
            return -std::numeric_limits<double>::infinity();
        }
        WH_UNREACHABLE("Bad special immedate double.");
        return 0.0;
    }

    inline double getImmDoubleValue() const {
        WH_ASSERT(isSpecialImmDouble());
        return isImmDoubleX() ? getImmDoubleXValue() : getImmDoubleHiLoValue();
    }

    inline Double *getHeapDouble() const {
        WH_ASSERT(isHeapDouble());
        return getPtr<Double>();
    }

    inline Magic getMagic() const {
        WH_ASSERT(isMagic());
        return static_cast<Magic>(removeTag());
    }

    inline int32_t getInt32() const {
        WH_ASSERT(isInt32());
        return static_cast<int32_t>(tagged_);
    }

    //
    // Friend functions
    //
    friend Value ObjectValue(Object *obj);
    friend Value NullValue();
    friend Value UndefinedValue();
    friend Value BooleanValue(bool b);
    friend Value StringValue(String *str);
    template <typename CharT>
    friend Value String8Value(unsigned length, const CharT *data);
    template <typename CharT>
    friend Value String16Value(unsigned length, const CharT *data);
    template <typename CharT>
    friend Value StringValue(unsigned length, const CharT *data);
    friend Value MagicValue(Magic m);
    friend Value IntegerValue(int32_t i);
    friend Value DoubleValue(Double *d);
    friend Value NaNValue();
    friend Value NegZeroValue();
    friend Value PosInfValue();
    friend Value NegInfValue();
    friend Value DoubleValue(double d);
};


inline Value
ObjectValue(Object *obj) {
    return Value::MakePtr(ValueTag::Object, obj);
}

inline Value
NullValue() {
    return Value::MakeTag(ValueTag::Null);
}

inline Value
UndefinedValue() {
    return Value::MakeTag(ValueTag::Undefined);
}

inline Value
BooleanValue(bool b) {
    return Value::MakeTagValue(ValueTag::Boolean, b);
}

inline Value
StringValue(String *str) {
    return Value::MakePtr(ValueTag::HeapString, str);
}

template <typename CharT>
inline Value
String8Value(unsigned length, const CharT *data) {
    WH_ASSERT(length < Value::ImmString8MaxLength);
    uint64_t val = length;
    for (unsigned i = 0; i < Value::ImmString8MaxLength; i++) {
        val <<= 8;
        if (i < length) {
            WH_ASSERT(data[i] == data[i] & 0xFFu);
            val |= data[i];
        }
    }
    return Value::MakeTagValue(ValueTag::ImmString8, val);
}

template <typename CharT>
inline Value
String16Value(unsigned length, const CharT *data) {
    WH_ASSERT(length < Value::ImmString16MaxLength);
    uint64_t val = length;
    for (unsigned i = 0; i < Value::ImmString16MaxLength; i++) {
        val <<= 16;
        if (i < length) {
            WH_ASSERT(data[i] == data[i] & 0xFFFFu);
            val |= data[i];
        }
    }
    return Value::MakeTagValue(ValueTag::ImmString16, val);
}

template <typename CharT>
inline Value
StringValue(unsigned length, const CharT *data)
{
    bool make8 = true;
    for (unsigned i = 0; i < length; i++) {
        WH_ASSERT(data[i] == data[i] & 0xFFFFu);
        if (data[i] != data[i] & 0xFFu) {
            make8 = false;
            break;
        }
    }

    return make8 ? String8Value(length, data) : String16Value(length, data);
}

inline Value
DoubleValue(Double *d) {
    return Value::MakePtr(ValueTag::HeapDouble, d);
}

inline Value
NegZeroValue() {
    return Value::MakeTagValue(ValueTag::ImmDoubleX,
                               static_cast<uint64_t>(Value::NegZeroVal));
}

inline Value
NaNValue() {
    return Value::MakeTagValue(ValueTag::ImmDoubleX,
                               static_cast<uint64_t>(Value::NaNVal));
}

inline Value
PosInfValue() {
    return Value::MakeTagValue(ValueTag::ImmDoubleX,
                               static_cast<uint64_t>(Value::PosInfVal));
}

inline Value
NegInfValue() {
    return Value::MakeTagValue(ValueTag::ImmDoubleX,
                               static_cast<uint64_t>(Value::NegInfVal));
}

inline Value
DoubleValue(double dval) {
    uint64_t ival = DoubleToInt(dval);
    WH_ASSERT(((ival <= Value::ImmDoublePosMax) &&
               (ival >= Value::ImmDoublePosMin)) ||
              ((ival <= Value::ImmDoubleNegMax) &&
               (ival >= Value::ImmDoubleNegMin)));
    return Value(RotateLeft(ival, 1));
}

inline Value
MagicValue(Magic magic) {
    return Value::MakeTagValue(ValueTag::Magic, static_cast<uint64_t>(magic));
}

inline Value
IntegerValue(int32_t ival) {
    return Value::MakeTagValue(ValueTag::Int32, static_cast<uint32_t>(ival));
}



} // namespace Whisper

#endif // WHISPER__VALUE_HPP

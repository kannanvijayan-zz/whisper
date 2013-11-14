
#include <limits>
#include <cmath>

#include "helpers.hpp"
#include "value.hpp"
#include "value_inlines.hpp"
#include "vm/string.hpp"
#include "vm/double.hpp"

namespace Whisper {

bool
IsValidValueTag(ValueTag tag)
{
    return tag == ValueTag::Object              ||
           tag == ValueTag::HeapString          ||
           tag == ValueTag::HeapDouble          ||
           tag == ValueTag::ImmDoubleLow        ||
           tag == ValueTag::ImmDoubleHigh       ||
           tag == ValueTag::ExtNumber           ||
           tag == ValueTag::StringAndRest;
}

unsigned
ValueTagNumber(ValueTag tag)
{
    WH_ASSERT(IsValidValueTag(tag));
    return ToUInt8(tag);
}

/*static*/ bool
Value::IsImmediateNumber(double dval)
{
    // Int32s are representable.
    if (ToInt32(dval) == dval)
        return true;

    // NaN is representable.
    if (DoubleIsNaN(dval))
        return true;

    // Infinity and -Infinity are representable.
    if (DoubleIsPosInf(dval) || DoubleIsNegInf(dval))
        return true;

    // 0.0 or -0.0 are representable.
    if (dval == 0)
        return true;

    // Look for exponents with high bits 100 or 011
    unsigned exponent = GetDoubleExponentField(dval);
    if (exponent >= 0x300 && exponent <= 0x4FF)
        return true;

    return false;
}

template <typename CharT>
static int32_t
ImmediateIndexValueHelper(uint32_t length, const CharT *str)
{
    if (length > Value::ImmIndexStringMaxLength || length == 0)
        return -1;

    if (str[0] == static_cast<CharT>('0')) {
        if (length == 1)
            return 0;
        return -1;
    }

    int32_t accum = 0;
    for (uint32_t i = 0; i < length; i++) {
        if (str[i] < static_cast<CharT>('0'))
            return -1;

        if (str[i] > static_cast<CharT>('9'))
            return -1;

        int32_t digit = ToInt32(str[i] - static_cast<CharT>('0'));

        if (accum > (INT32_MAX / 10))
            return -1;

        accum *= 10;
        accum += digit;
        if (accum < 0)
            return -1;
    }

    return accum;
}

/*static*/ int32_t
Value::ImmediateIndexValue(uint32_t length, const uint8_t *str)
{
    return ImmediateIndexValueHelper<uint8_t>(length, str);
}

/*static*/ int32_t
Value::ImmediateIndexValue(uint32_t length, const uint16_t *str)
{
    return ImmediateIndexValueHelper<uint16_t>(length, str);
}

/*static*/ bool
Value::IsImmediateIndexString(uint32_t length, const uint8_t *str)
{
    return ImmediateIndexValue(length, str) == -1;
}

/*static*/ bool
Value::IsImmediateIndexString(uint32_t length, const uint16_t *str)
{
    return ImmediateIndexValue(length, str) == -1;
}

Value::Value() : tagged_(Invalid) {}

// Raw uint64_t constructor is private.
Value::Value(uint64_t tagged) : tagged_(tagged)
{
#if defined(ENABLE_DEBUG)
    WH_ASSERT(isValid());
#endif // defined(ENABLE_DEBUG)
}

ValueTag
Value::getTag() const
{
    ValueTag tag = static_cast<ValueTag>(tagged_ & TagMask);
    WH_ASSERT(IsValidValueTag(tag));
    return tag;
}

bool
Value::checkTag(ValueTag tag) const
{
    return getTag() == tag;
}


uint64_t
Value::raw() const
{
    return tagged_;
}


/*static*/ Value
Value::Undefined()
{
    return Value(UndefinedVal);
}

/*static*/ Value
Value::Int32(int32_t value)
{
    return Value((ToUInt64(value) << Int32Shift) | Int32Code);
}

/*static*/ Value
Value::Double(double dval)
{
    WH_ASSERT(IsImmediateNumber(dval));
    WH_ASSERT_IF(dval == 0.0, GetDoubleSign(dval));

    if (dval != dval)
        return NaN();

    if (dval == std::numeric_limits<double>::infinity())
        return PosInf();

    if (dval == -std::numeric_limits<double>::infinity())
        return NegInf();

    if (dval == 0 && GetDoubleSign(dval))
        return NegZero();

    // Otherwise, rotate the double value.
    Value result = Value(RotateLeft(DoubleToInt(dval), 4));
    WH_ASSERT(result.isImmDoubleLow() || result.isImmDoubleHigh());
    return result;
}

/*static*/ Value
Value::Number(double dval)
{
    WH_ASSERT(IsImmediateNumber(dval));

    if (ToInt32(dval) == dval)
        return Int32(dval);

    return Double(dval);
}

/*static*/ Value
Value::HeapDouble(VM::HeapDouble *dbl)
{
    WH_ASSERT(dbl != nullptr);
    WH_ASSERT(IsPtrAligned(dbl, 1u << TagBits));
    return Value(PtrToWord(dbl) | ValueTagNumber(ValueTag::HeapDouble));
}

/*static*/ Value
Value::NaN()
{
    return Value(NaNVal);
}

/*static*/ Value
Value::PosInf()
{
    return Value(PosInfVal);
}

/*static*/ Value
Value::NegInf()
{
    return Value(NegInfVal);
}

/*static*/ Value
Value::NegZero()
{
    return Value(NegZeroVal);
}

/*static*/ Value
Value::ImmString8(unsigned length, const uint8_t *data)
{
    WH_ASSERT(length <= ImmString8MaxLength);
    uint64_t val = ImmString8Code | (length << ImmString8LengthShift);
    for (unsigned i = 0; i < length; i++)
        val |= data[i] << (ImmString8DataShift + (i*8));
    return Value(val);
}

/*static*/ Value
Value::ImmString16(unsigned length, const uint16_t *data)
{
    WH_ASSERT(length <= ImmString16MaxLength);
    uint64_t val = ImmString16Code | (length << ImmString16LengthShift);
    for (unsigned i = 0; i < length; i++)
        val |= data[i] << (ImmString16DataShift + (i*16));
    return Value(val);
}

/*static*/ Value
Value::ImmIndexString(int32_t idx)
{
    WH_ASSERT(idx >= 0);
    uint64_t val = ImmIndexStringCode |
                   (ToUInt64(idx) << ImmIndexStringDataShift);
    return Value(val);
}

/*static*/ Value
Value::HeapString(VM::HeapString *str)
{
    WH_ASSERT(str != nullptr);
    WH_ASSERT(IsPtrAligned(str, 1u << TagBits));
    return Value(PtrToWord(str) | ValueTagNumber(ValueTag::HeapString));
}



#if defined(ENABLE_DEBUG)
bool
Value::isValid() const
{
    ValueTag tag = getTag();
    switch (tag) {
      case ValueTag::Object:
      case ValueTag::HeapString:
      case ValueTag::HeapDouble:
        return (tagged_ & ~TagMask) != 0u;

      case ValueTag::ImmDoubleLow:
      case ValueTag::ImmDoubleHigh:
        return true;

      case ValueTag::ExtNumber:
        if ((tagged_ & Int32Mask) == Int32Code)
            return true;

        return tagged_ == NaNVal || tagged_ == NegInfVal ||
               tagged_ == PosInfVal || tagged_ == NegZeroVal;

      case ValueTag::StringAndRest:
        if (((tagged_ & ImmString8Mask) == ImmString8Code) ||
            ((tagged_ & ImmString16Mask) == ImmString16Code) ||
            ((tagged_ & ImmIndexStringMask) == ImmIndexStringCode))
        {
            return true;
        }
        return tagged_ == UndefinedVal ||
               tagged_ == NullVal ||
               tagged_ == FalseVal ||
               tagged_ == TrueVal;

      default:
        WH_UNREACHABLE("Invalid ValueTag.");
        return false;
    }
}
#endif // defined(ENABLE_DEBUG)

ValueType
Value::type() const
{
    switch (getTag()) {
      case ValueTag::Object:
        return ValueType::Object;

      case ValueTag::HeapString:
        return ValueType::String;

      case ValueTag::HeapDouble:
      case ValueTag::ImmDoubleLow:
      case ValueTag::ImmDoubleHigh:
      case ValueTag::ExtNumber:
        return ValueType::Number;

      case ValueTag::StringAndRest:
        if (((tagged_ & ImmString8Mask) == ImmString8Code) ||
            ((tagged_ & ImmString16Mask) == ImmString16Code) ||
            ((tagged_ & ImmIndexStringMask) == ImmIndexStringCode))
        {
            return ValueType::String;
        }

        if (tagged_ == UndefinedVal)
            return ValueType::Undefined;

        if (tagged_ == NullVal)
            return ValueType::Null;

        if (tagged_ == FalseVal)
            return ValueType::Boolean;

        if (tagged_ == TrueVal)
            return ValueType::Boolean;

      default:
        WH_UNREACHABLE("Invalid ValueTag.");
        return ValueType::INVALID;
    }
}

bool
Value::isObject() const
{
    return checkTag(ValueTag::Object);
}

bool
Value::isHeapString() const
{
    return checkTag(ValueTag::HeapString);
}

bool
Value::isHeapDouble() const
{
    return checkTag(ValueTag::HeapDouble);
}

bool
Value::isImmDoubleLow() const
{
    return checkTag(ValueTag::ImmDoubleLow);
}

bool
Value::isImmDoubleHigh() const
{
    return checkTag(ValueTag::ImmDoubleHigh);
}

bool
Value::isNaN() const
{
    return (tagged_ & ExtNumberMask) == NaNVal;
}

bool
Value::isNegInf() const
{
    return (tagged_ & ExtNumberMask) == NegInfVal;
}

bool
Value::isPosInf() const
{
    return (tagged_ & ExtNumberMask) == PosInfVal;
}

bool
Value::isNegZero() const
{
    return (tagged_ & ExtNumberMask) == NegZeroVal;
}

bool
Value::isInt32() const
{
    return (tagged_ & ExtNumberMask) == Int32Code;
}

bool
Value::isImmString8() const
{
    return (tagged_ & ImmString8Mask) == ImmString8Code;
}

bool
Value::isImmString16() const
{
    return (tagged_ & ImmString16Mask) == ImmString16Code;
}

bool
Value::isImmIndexString() const
{
    return (tagged_ & ImmIndexStringMask) == ImmIndexStringCode;
}

bool
Value::isUndefined() const
{
    return (tagged_ & RestMask) == UndefinedVal;
}

bool
Value::isNull() const
{
    return (tagged_ & RestMask) == NullVal;
}

bool
Value::isFalse() const
{
    return (tagged_ & RestMask) == FalseVal;
}

bool
Value::isTrue() const
{
    return (tagged_ & RestMask) == TrueVal;
}




bool
Value::isHeapThing() const
{
    return getTag() <= ValueTag::HeapDouble;
}

bool
Value::isPrimitive() const
{
    return getTag() >= ValueTag::HeapString;
}

bool
Value::isImmediate() const
{
    return getTag() >= ValueTag::ImmDoubleLow;
}


bool
Value::isNumber() const
{
    return getTag() >= ValueTag::HeapDouble &&
           getTag() <= ValueTag::ExtNumber;
}

bool
Value::isBoolean() const
{
    return (tagged_ & BoolMask) == BoolCode;
}

bool
Value::isImmString() const
{
    return isImmString8() || isImmString16() || isImmIndexString();
}

bool
Value::isString() const
{
    return isHeapString() || isImmString();
}



VM::Object *
Value::objectPtr() const
{
    WH_ASSERT(isObject());
    return reinterpret_cast<VM::Object *>(tagged_);
}

VM::HeapString *
Value::heapStringPtr() const
{
    WH_ASSERT(isHeapString());
    unsigned xorMask = ValueTagNumber(ValueTag::HeapString);
    VM::HeapString *s = reinterpret_cast<VM::HeapString *>(tagged_ ^ xorMask);
    WH_ASSERT(s->isValidString());
    return s;
}

VM::HeapDouble *
Value::heapDoublePtr() const
{
    WH_ASSERT(isHeapDouble());
    unsigned xorMask = ValueTagNumber(ValueTag::HeapDouble);
    VM::HeapDouble *s = reinterpret_cast<VM::HeapDouble *>(tagged_ ^ xorMask);
    WH_ASSERT(s->isHeapDouble());
    return s;
}

int32_t
Value::int32Value() const
{
    WH_ASSERT(isInt32());
    return ToInt32(tagged_ >> Int32Shift);
}

double
Value::numberValue() const
{
    WH_ASSERT(isNumber());

    if (isInt32())
        return int32Value();

    if (isNaN())
        return std::numeric_limits<double>::quiet_NaN();

    if (isNegInf())
        return -std::numeric_limits<double>::infinity();

    if (isPosInf())
        return std::numeric_limits<double>::infinity();

    if (isNegZero())
        return -static_cast<double>(0.0);

    if (isImmDoubleLow() || isImmDoubleHigh())
        return IntToDouble(RotateRight<uint64_t>(tagged_, 4));

    WH_ASSERT(isHeapDouble());
    return heapDoublePtr()->value();
}

unsigned
Value::immString8Length() const
{
    WH_ASSERT(isImmString8());
    return (tagged_ >> ImmString8LengthShift) & ImmString8LengthMask;
}

uint8_t
Value::getImmString8Char(unsigned idx) const
{
    WH_ASSERT(idx < immString8Length());
    return (tagged_ >> (ImmString8DataShift + (idx * 8))) & 0xFFu;
}

unsigned
Value::immString16Length() const
{
    WH_ASSERT(isImmString16());
    return (tagged_ >> ImmString16LengthShift) & ImmString16LengthMask;
}

uint16_t
Value::getImmString16Char(unsigned idx) const
{
    WH_ASSERT(idx < immString16Length());
    return (tagged_ >> (ImmString16DataShift + (idx * 16))) & 0xFFFFu;
}

int32_t
Value::immIndexStringValue() const
{
    WH_ASSERT(isImmIndexString());
    return tagged_ >> ImmIndexStringDataShift;
}

unsigned
Value::immIndexStringLength() const
{
    WH_ASSERT(isImmIndexString());
    int32_t val = immIndexStringValue();
    WH_ASSERT(val > 0);
    if (val < 10)
        return 1;
    if (val < 100)
        return 2;
    if (val < 1000)
        return 3;
    if (val < 10000)
        return 4;
    if (val < 100000)
        return 5;
    if (val < 1000000)
        return 6;
    if (val < 10000000)
        return 7;
    if (val < 100000000)
        return 8;
    if (val < 1000000000)
        return 9;
    return 10;
}

uint8_t
Value::getImmIndexStringChar(unsigned idx) const
{
    WH_ASSERT(idx < immIndexStringLength());
    int32_t val = immIndexStringValue();
    return ToUInt8('0') + ((val / (idx * 10)) % 10);
}

unsigned
Value::immStringLength() const
{
    WH_ASSERT(isImmString8() || isImmString16() || isImmIndexString());

    if (isImmString8())
        return immString8Length();

    if (isImmString16())
        immString16Length();

    return immIndexStringLength();
}

uint16_t
Value::getImmStringChar(unsigned idx) const
{
    WH_ASSERT(isImmString8() || isImmString16() || isImmIndexString());

    if (isImmString8())
        return getImmString8Char(idx);

    if (isImmString16())
        return getImmString16Char(idx);

    return getImmIndexStringChar(idx);
}


} // namespace Whisper

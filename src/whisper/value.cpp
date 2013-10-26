#include "helpers.hpp"
#include "value.hpp"
#include "value_inlines.hpp"
#include "vm/string.hpp"

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


/*static*/
Value
Value::Undefined()
{
    return Value(UndefinedVal);
}

/*static*/
Value
Value::Int32(int32_t value)
{
    return Value((ToUInt64(value) << Int32Shift) | Int32Code);
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
        if (((tagged_ & ImmStringMask) == ImmString8Code) ||
            ((tagged_ & ImmStringMask) == ImmString16Code))
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
        if (((tagged_ & ImmStringMask) == ImmString8Code) ||
            ((tagged_ & ImmStringMask) == ImmString16Code))
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
    return (tagged_ & ImmStringMask) == ImmString8Code;
}

bool
Value::isImmString16() const
{
    return (tagged_ & ImmStringMask) == ImmString16Code;
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
Value::isString() const
{
    return isHeapString() || isImmString8() || isImmString16();
}



VM::Object *
Value::objectPtr() const
{
    WH_ASSERT(isObject());
    return reinterpret_cast<VM::Object *>(tagged_);
}

VM::HeapString *
Value::getHeapString() const
{
    WH_ASSERT(isHeapString());
    unsigned xorMask = ValueTagNumber(ValueTag::HeapString);
    VM::HeapString *s = reinterpret_cast<VM::HeapString *>(tagged_ ^ xorMask);
    WH_ASSERT(s->isValidString());
    return s;
}

int32_t
Value::int32Value() const
{
    WH_ASSERT(isInt32());
    return ToInt32(tagged_ >> Int32Shift);
}

unsigned
Value::immString8Length() const
{
    WH_ASSERT(isImmString8());
    return (tagged_ >> ImmStringLengthShift) & ImmString8LengthMask;
}

uint8_t
Value::getImmString8Char(unsigned idx) const
{
    WH_ASSERT(isImmString8());
    WH_ASSERT(idx < immString8Length());
    return (tagged_ >> (ImmString8DataShift + (idx * 8))) & 0xFFu;
}

unsigned
Value::immString16Length() const
{
    WH_ASSERT(isImmString16());
    return (tagged_ >> ImmStringLengthShift) & ImmString16LengthMask;
}

uint16_t
Value::getImmString16Char(unsigned idx) const
{
    WH_ASSERT(isImmString16());
    WH_ASSERT(idx < immString16Length());
    return (tagged_ >> (ImmString16DataShift + (idx * 16))) & 0xFFFFu;
}

unsigned
Value::immStringLength() const
{
    WH_ASSERT(isImmString8() || isImmString16());

    if (isImmString8())
        return immString8Length();

    return immString16Length();
}

uint16_t
Value::getImmStringChar(unsigned idx) const
{
    WH_ASSERT(isImmString8() || isImmString16());

    if (isImmString8())
        return getImmString8Char(idx);

    return getImmString16Char(idx);
}


} // namespace Whisper

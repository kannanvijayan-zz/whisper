#include "helpers.hpp"
#include "value.hpp"
#include "value_inlines.hpp"

namespace Whisper {


Value::Value() : tagged_(Invalid) {}

// Raw uint64_t constructor is private.
Value::Value(uint64_t tagged) : tagged_(tagged) {}

ValueTag
Value::getTag() const
{
    return static_cast<ValueTag>(tagged_ >> TagShift);
}

bool
Value::checkTag(ValueTag tag) const
{
    return getTag() == tag;
}

uint64_t
Value::removeTag() const
{
    return (tagged_ << TagBits) >> TagBits;
}

/*static*/ Value
Value::MakeTag(ValueTag tag)
{
    WH_ASSERT(IsValidValueTag(tag));
    return Value(UInt64(tag) << TagShift);
}

/*static*/ Value
Value::MakeTagValue(ValueTag tag, uint64_t ival)
{
    WH_ASSERT(IsValidValueTag(tag));
    WH_ASSERT(ival <= (~UInt64(0) >> TagBits));
    return Value(UInt64(tag) << TagShift);
}

#if defined(ENABLE_DEBUG)
bool
Value::isValid() const
{
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

ValueType
Value::type() const
{
    switch (getTag()) {
      case ValueTag::Object:
        return ValueType::Object;

      case ValueTag::Null:
        return ValueType::Null;

      case ValueTag::Undefined:
        return ValueType::Undefined;

      case ValueTag::Boolean:
        return ValueType::Boolean;

      case ValueTag::HeapString:
      case ValueTag::ImmString8:
      case ValueTag::ImmString16:
        return ValueType::String;

      case ValueTag::ImmDoubleLow:
      case ValueTag::ImmDoubleHigh:
      case ValueTag::ImmDoubleX:
      case ValueTag::HeapDouble:
      case ValueTag::Int32:
        return ValueType::Number;
      default:
        break;
    }
    WH_UNREACHABLE("Value does not have legitimate ValueType.");
    return ValueType::INVALID;
}

bool
Value::isObject() const
{
    return checkTag(ValueTag::Object);
}

bool
Value::isNativeObject() const
{
    return isObject() &&
           ((tagged_ >> PtrTypeShift) & PtrTypeMask) == PtrType_Native;
}

bool
Value::isForeignObject() const
{
    return isObject() &&
           ((tagged_ >> PtrTypeShift) & PtrTypeMask) == PtrType_Foreign;
}

bool
Value::isNull() const
{
    return checkTag(ValueTag::Null);
}

bool
Value::isUndefined() const
{
    return checkTag(ValueTag::Undefined);
}

bool
Value::isBoolean() const
{
    return checkTag(ValueTag::Boolean);
}

bool
Value::isHeapString() const
{
    return checkTag(ValueTag::HeapString);
}

bool
Value::isImmString8() const
{
    return checkTag(ValueTag::ImmString8);
}

bool
Value::isImmString16() const
{
    return checkTag(ValueTag::ImmString16);
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
Value::isImmDoubleX() const
{
    return checkTag(ValueTag::ImmDoubleX);
}

bool
Value::isNegZero() const
{
    return isImmDoubleX() && (tagged_ & 0xFu) == NegZeroVal;
}

bool
Value::isNaN() const
{
    return isImmDoubleX() && (tagged_ & 0xFu) == NaNVal;
}

bool
Value::isPosInf() const
{
    return isImmDoubleX() && (tagged_ & 0xFu) == PosInfVal;
}

bool
Value::isNegInf() const
{
    return isImmDoubleX() && (tagged_ & 0xFu) == NegInfVal;
}

bool
Value::isHeapDouble() const
{
    return checkTag(ValueTag::HeapDouble);
}

bool
Value::isInt32() const
{
    return checkTag(ValueTag::Int32);
}

bool
Value::isMagic() const
{
    return checkTag(ValueTag::Magic);
}

bool
Value::isString() const
{
    return isImmString8() || isImmString16() || isHeapString();
}

bool
Value::isImmString() const
{
    return isImmString8() || isImmString16();
}

bool
Value::isNumber() const
{
    return isImmDoubleLow() || isImmDoubleHigh() || isImmDoubleX() ||
           isHeapDouble()   || isInt32();
}

bool
Value::isDouble() const
{
    return isImmDoubleLow() || isImmDoubleHigh() || isImmDoubleX() ||
           isHeapDouble();
}

bool
Value::isSpecialImmDouble() const
{
   return isImmDoubleLow() || isImmDoubleHigh() || isImmDoubleX();
}

bool
Value::isRegularImmDouble() const
{
   return isImmDoubleLow() || isImmDoubleHigh();
}

bool
Value::isWeakPointer() const
{
    WH_ASSERT(isHeapDouble() || isHeapString() || isObject());
    return tagged_ & WeakMask;
}


VM::UntypedHeapThing *
Value::getAnyNativeObject() const
{
    WH_ASSERT(isNativeObject());
    return getPtr<VM::UntypedHeapThing>();
}

bool
Value::getBoolean() const
{
    WH_ASSERT(isBoolean());
    return tagged_ & 0x1;
}

VM::HeapString *
Value::getHeapString() const
{
    WH_ASSERT(isHeapString());
    return getPtr<VM::HeapString>();
}

unsigned
Value::immString8Length() const
{
    WH_ASSERT(isImmString8());
    return (tagged_ >> ImmString8LengthShift) & ImmString8LengthMaskLow;
}

uint8_t
Value::getImmString8Char(unsigned idx) const
{
    WH_ASSERT(isImmString8());
    WH_ASSERT(idx < immString8Length());
    return (tagged_ >> (48 - (idx*8))) & 0xFFu;
}

unsigned
Value::immString16Length() const
{
    WH_ASSERT(isImmString16());
    return (tagged_ >> ImmString16LengthShift) & ImmString16LengthMaskLow;
}

uint16_t
Value::getImmString16Char(unsigned idx) const
{
    WH_ASSERT(isImmString16());
    WH_ASSERT(idx < immString16Length());
    return (tagged_ >> (32 - (idx*16))) & 0xFFFFu;
}

unsigned
Value::immStringLength() const
{
    WH_ASSERT(isImmString());
    return isImmString8() ? immString8Length() : immString16Length();
}

uint16_t
Value::getImmStringChar(unsigned idx) const
{
    WH_ASSERT(isImmString());
    return isImmString8() ? getImmString8Char(idx) : getImmString16Char(idx);
}

double
Value::getImmDoubleHiLoValue() const
{
    WH_ASSERT(isImmDoubleHigh() || isImmDoubleLow());
    return IntToDouble(RotateRight(tagged_, 1));
}

double
Value::getImmDoubleXValue() const
{
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

double
Value::getImmDoubleValue() const
{
    WH_ASSERT(isSpecialImmDouble());
    return isImmDoubleX() ? getImmDoubleXValue() : getImmDoubleHiLoValue();
}

VM::HeapDouble *
Value::getHeapDouble() const
{
    WH_ASSERT(isHeapDouble());
    return getPtr<VM::HeapDouble>();
}

uint64_t
Value::getMagicInt() const
{
    WH_ASSERT(isMagic());
    return removeTag();
}

Magic
Value::getMagic() const
{
    WH_ASSERT(isMagic());
    return static_cast<Magic>(removeTag());
}

int32_t
Value::getInt32() const
{
    WH_ASSERT(isInt32());
    return static_cast<int32_t>(tagged_);
}

Value
ForeignObjectValue(void *obj)
{
    Value val = Value::MakePtr(ValueTag::Object, obj);
    val.tagged_ |= (UInt64(Value::PtrType_Foreign) << Value::PtrTypeShift);
    return val;
}

Value
WeakForeignObjectValue(void *obj)
{
    Value val = ForeignObjectValue(obj);
    val.tagged_ |= Value::WeakMask;
    return val;
}

Value
NullValue()
{
    return Value::MakeTag(ValueTag::Null);
}

Value
UndefinedValue()
{
    return Value::MakeTag(ValueTag::Undefined);
}

Value
BooleanValue(bool b)
{
    return Value::MakeTagValue(ValueTag::Boolean, b);
}

Value
StringValue(VM::HeapString *str)
{
    WH_ASSERT(str != nullptr);
    return Value::MakePtr(ValueTag::HeapString, str);
}

Value
DoubleValue(VM::HeapDouble *d)
{
    WH_ASSERT(d != nullptr);
    return Value::MakePtr(ValueTag::HeapDouble, d);
}

Value
NegZeroValue()
{
    return Value::MakeTagValue(ValueTag::ImmDoubleX, UInt64(Value::NegZeroVal));
}

Value
NaNValue()
{
    return Value::MakeTagValue(ValueTag::ImmDoubleX, UInt64(Value::NaNVal));
}

Value
PosInfValue()
{
    return Value::MakeTagValue(ValueTag::ImmDoubleX, UInt64(Value::PosInfVal));
}

Value
NegInfValue()
{
    return Value::MakeTagValue(ValueTag::ImmDoubleX, UInt64(Value::NegInfVal));
}

Value
DoubleValue(double dval)
{
    uint64_t ival = DoubleToInt(dval);
    WH_ASSERT(((ival <= Value::ImmDoublePosMax) &&
               (ival >= Value::ImmDoublePosMin)) ||
              ((ival <= Value::ImmDoubleNegMax) &&
               (ival >= Value::ImmDoubleNegMin)));
    return Value(RotateLeft(ival, 1));
}

Value
MagicValue(uint64_t val)
{
    WH_ASSERT((val & Value::TagMaskHigh) == 0);
    return Value::MakeTagValue(ValueTag::Magic, val);
}

Value
MagicValue(Magic magic)
{
    return Value::MakeTagValue(ValueTag::Magic, UInt64(magic));
}

Value
IntegerValue(int32_t ival)
{
    // Cast to uint32_t so value doesn't get sign extended when casting
    // to uint64_t
    return Value::MakeTagValue(ValueTag::Int32, UInt32(ival));
}


} // namespace Whisper

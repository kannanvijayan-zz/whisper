#include "helpers.hpp"
#include "value.hpp"
#include "value_inlines.hpp"

namespace Whisper {


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
    ValueTag tag = static_cast<ValueTag>(tagged_ >> RegularTagShift);
    switch (tag) {
        case ValueTag::Object:
        case ValueTag::Null:
        case ValueTag::Undefined:
        case ValueTag::Boolean:
        case ValueTag::SpecialImmDouble:
        case ValueTag::HeapDouble:
        case ValueTag::HeapString:
        case ValueTag::ImmString8:
        case ValueTag::ImmString16:
        case ValueTag::Int32:
        case ValueTag::Magic:
            return tag;

        case ValueTag::PosImmDoubleBig:
        case ValueTag::PosImmDoubleBig_2:
        case ValueTag::PosImmDoubleBig_3:
        case ValueTag::PosImmDoubleBig_4:
            return ValueTag::PosImmDoubleBig;

        case ValueTag::PosImmDoubleSmall:
        case ValueTag::PosImmDoubleSmall_2:
        case ValueTag::PosImmDoubleSmall_3:
        case ValueTag::PosImmDoubleSmall_4:
            return ValueTag::PosImmDoubleSmall;

        case ValueTag::NegImmDoubleBig:
        case ValueTag::NegImmDoubleBig_1:
        case ValueTag::NegImmDoubleBig_2:
        case ValueTag::NegImmDoubleBig_3:
            return ValueTag::NegImmDoubleBig;

        case ValueTag::NegImmDoubleSmall:
        case ValueTag::NegImmDoubleSmall_2:
        case ValueTag::NegImmDoubleSmall_3:
        case ValueTag::NegImmDoubleSmall_4:
            return ValueTag::NegImmDoubleSmall;

        case ValueTag::UNUSED_11:
        case ValueTag::UNUSED_12:
        case ValueTag::UNUSED_13:
        case ValueTag::UNUSED_1E:
        case ValueTag::UNUSED_1F:
        default:
            WH_UNREACHABLE("Invalid value.");
            return ValueTag::UNUSED_1F;
    }
}

bool
Value::checkTag(ValueTag tag) const
{
    return getTag() == tag;
}

/*static*/ Value
Value::MakeTag(ValueTag tag)
{
    WH_ASSERT(IsValidValueTag(tag));
    return Value(UInt64(tag) << RegularTagShift);
}

/*static*/ Value
Value::MakeTagValue(ValueTag tag, uint64_t ival)
{
    WH_ASSERT(IsValidValueTag(tag));
    WH_ASSERT(ival <= (~UInt64(0) >> RegularTagBits));
    return Value(ival | (UInt64(tag) << RegularTagShift));
}

#if defined(ENABLE_DEBUG)
bool
Value::isValid() const
{
    // If heap thing, pointer can't be zero.
    if (isHeapString() || isHeapDouble() || isObject())
        return getPtr() != nullptr;

    // Otherwise, check for legit tag.
    return IsValidValueTag(getTag());
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

      case ValueTag::PosImmDoubleSmall:
      case ValueTag::PosImmDoubleBig:
      case ValueTag::NegImmDoubleSmall:
      case ValueTag::NegImmDoubleBig:
      case ValueTag::SpecialImmDouble:
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
           ((tagged_ >> PtrTypeShift) & PtrTypeMaskLow) == PtrType_Native;
}

bool
Value::isForeignObject() const
{
    return isObject() &&
           ((tagged_ >> PtrTypeShift) & PtrTypeMaskLow) == PtrType_Foreign;
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
Value::isPosImmDoubleSmall() const
{
    return checkTag(ValueTag::PosImmDoubleSmall);
}

bool
Value::isPosImmDoubleBig() const
{
    return checkTag(ValueTag::PosImmDoubleBig);
}

bool
Value::isNegImmDoubleSmall() const
{
    return checkTag(ValueTag::NegImmDoubleSmall);
}

bool
Value::isNegImmDoubleBig() const
{
    return checkTag(ValueTag::NegImmDoubleBig);
}

bool
Value::isSpecialImmDouble() const
{
    return checkTag(ValueTag::SpecialImmDouble);
}

bool
Value::isNegZero() const
{
    return isSpecialImmDouble() &&
           (tagged_ & SpecialImmDoubleValueMask) == NegZeroVal;
}

bool
Value::isNaN() const
{
    return isSpecialImmDouble() &&
           (tagged_ & SpecialImmDoubleValueMask) == NaNVal;
}

bool
Value::isPosInf() const
{
    return isSpecialImmDouble() &&
           (tagged_ & SpecialImmDoubleValueMask) == PosInfVal;
}

bool
Value::isNegInf() const
{
    return isSpecialImmDouble() &&
           (tagged_ & SpecialImmDoubleValueMask) == NegInfVal;
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
    return isRegularImmDouble() || isSpecialImmDouble() ||
           isHeapDouble()       || isInt32();
}

bool
Value::isDouble() const
{
    return isImmDouble() || isHeapDouble();
}

bool
Value::isImmDouble() const
{
   return isRegularImmDouble() || isSpecialImmDouble();
}

bool
Value::isRegularImmDouble() const
{
   return isPosImmDoubleSmall() || isPosImmDoubleBig() ||
          isNegImmDoubleSmall() || isNegImmDoubleBig();
}

bool
Value::isWeakPointer() const
{
    WH_ASSERT(isHeapDouble() || isHeapString() || isObject());
    return tagged_ & WeakMask;
}


VM::HeapThing *
Value::getAnyNativeObject() const
{
    WH_ASSERT(isNativeObject());
    return getPtr<VM::HeapThing>();
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
Value::getRegularImmDoubleValue() const
{
    WH_ASSERT(isRegularImmDouble());
    return IntToDouble(tagged_);
}

double
Value::getSpecialImmDoubleValue() const
{
    WH_ASSERT(isSpecialImmDouble());
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
    WH_ASSERT(isImmDouble());
    return isRegularImmDouble() ? getRegularImmDoubleValue()
                                : getSpecialImmDoubleValue();
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
    return tagged_ & ~RegularTagMaskHigh;
}

int32_t
Value::getInt32() const
{
    WH_ASSERT(isInt32());
    return static_cast<int32_t>(tagged_);
}

Value
NativeObjectValue(VM::HeapThing *obj)
{
    WH_ASSERT(obj != nullptr);
    Value val = Value::MakePtr(ValueTag::Object, obj);
    return val;
}

Value
WeakNativeObjectValue(VM::HeapThing *obj)
{
    WH_ASSERT(obj != nullptr);
    Value val = NativeObjectValue(obj);
    val.tagged_ |= Value::WeakMask;
    return val;
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
    return Value::MakeTagValue(ValueTag::SpecialImmDouble,
                               UInt64(Value::NegZeroVal));
}

Value
NaNValue()
{
    return Value::MakeTagValue(ValueTag::SpecialImmDouble,
                               UInt64(Value::NaNVal));
}

Value
PosInfValue()
{
    return Value::MakeTagValue(ValueTag::SpecialImmDouble,
                               UInt64(Value::PosInfVal));
}

Value
NegInfValue()
{
    return Value::MakeTagValue(ValueTag::SpecialImmDouble,
                               UInt64(Value::NegInfVal));
}

Value
DoubleValue(double dval)
{
    uint64_t ival = DoubleToInt(dval);

#if defined(ENABLE_DEBUG)
    uint64_t dtag = (ival >> Value::DoubleTagShift) & Value::DoubleTagMaskLow;
    WH_ASSERT(dtag == Value::DoubleTag_PosSmall ||
              dtag == Value::DoubleTag_PosBig   ||
              dtag == Value::DoubleTag_NegSmall ||
              dtag == Value::DoubleTag_NegBig);
#endif // defined(ENABLE_DEBUG)

    return Value(ival);
}

Value
MagicValue(uint64_t val)
{
    WH_ASSERT((val & Value::RegularTagMaskHigh) == 0);
    return Value::MakeTagValue(ValueTag::Magic, val);
}

Value
IntegerValue(int32_t ival)
{
    // Cast to uint32_t so value doesn't get sign extended when casting
    // to uint64_t
    return Value::MakeTagValue(ValueTag::Int32, UInt32(ival));
}


} // namespace Whisper

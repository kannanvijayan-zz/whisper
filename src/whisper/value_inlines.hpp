#ifndef WHISPER__VALUE_INLINES_HPP
#define WHISPER__VALUE_INLINES_HPP

#include "value.hpp"

namespace Whisper {


template <typename T>
inline T *
Value::getPtr() const
{
    WH_ASSERT(isObject() || isHeapString() || isHeapDouble());
    return reinterpret_cast<T *>(
            (static_cast<int64_t>(tagged_ << PtrHighBits)) >> PtrHighBits);
}

template <typename T>
/*static*/ inline Value
Value::MakePtr(ValueTag tag, T *ptr)
{
    WH_ASSERT(tag == ValueTag::Object || tag == ValueTag::HeapString ||
              tag == ValueTag::HeapDouble);
    uint64_t ptrval = reinterpret_cast<uint64_t>(ptr);
    ptrval &= ~TagMaskHigh;
    ptrval |= UInt64(tag) << TagShift;
    return Value(ptrval);
}

template <typename T>
inline bool
Value::isNativeObjectOf() const
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
    if (!isNativeObject())
        return false;
    return getPtr<T>()->type() == T::Type;
}

template <typename T>
inline T *
Value::getNativeObject() const
{
    WH_ASSERT(isNativeObject());
    WH_ASSERT(getPtr<T>()->type() == T::Type);
    return getPtr<T>();
}

template <typename T>
inline T *
Value::getForeignObject() const
{
    WH_ASSERT(isForeignObject());
    return getPtr<T>();
}

template <typename CharT>
inline uint32_t
Value::readImmString8(CharT *buf) const
{
    WH_ASSERT(isImmString8());
    uint32_t length = immString8Length();
    for (uint32_t i = 0; i < length; i++)
        buf[i] = (tagged_ >> (48 - (8 * i))) & 0xFFu;
    return length;
}

template <typename CharT>
inline uint32_t
Value::readImmString16(CharT *buf) const
{
    WH_ASSERT(isImmString16());
    uint32_t length = immString16Length();
    for (uint32_t i = 0; i < length; i++)
        buf[i] = (tagged_ >> (32 - (16 * i))) & 0xFFu;
    return length;
}

template <typename CharT>
inline unsigned
Value::readImmString(CharT *buf) const
{
    WH_ASSERT(isImmString());
    return isImmString8() ? readImmString8<CharT>(buf)
                          : readImmString16<CharT>(buf);
}

template <typename T>
inline Value
NativeObjectValue(T *obj)
{
    WH_ASSERT(obj != nullptr);
    WH_ASSERT(T::Type == obj->type());
    Value val = Value::MakePtr(ValueTag::Object, obj);
    return val;
}

template <typename T>
inline Value
WeakNativeObjectValue(T *obj)
{
    WH_ASSERT(obj != nullptr);
    Value val = NativeObjectValue<T>(obj);
    val.tagged_ |= Value::WeakMask;
    return val;
}

template <typename CharT>
inline Value
String8Value(unsigned length, const CharT *data)
{
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
String16Value(unsigned length, const CharT *data)
{
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


} // namespace Whisper

#endif // WHISPER__VALUE_INLINES_HPP

#ifndef WHISPER__VM__HEAP_THING_INLINES_HPP
#define WHISPER__VM__HEAP_THING_INLINES_HPP

#include "vm/heap_thing.hpp"

namespace Whisper {
namespace VM {


//
// HeapThingWrapper<typename T>
//

template <typename T>
template <typename... T_ARGS>
inline
HeapThingWrapper<T>::HeapThingWrapper(uint32_t cardNo, uint32_t size,
                                      T_ARGS... tArgs)
  : header_(T::Type, cardNo, size),
    payload_(tArgs...)
{}

template <typename T>
inline const HeapThingHeader &
HeapThingWrapper<T>::header() const
{
    return header_;
}

template <typename T>
inline HeapThingHeader &
HeapThingWrapper<T>::header()
{
    return header_;
}

template <typename T>
inline const HeapThingHeader *
HeapThingWrapper<T>::headerPointer() const
{
    return &header_;
}

template <typename T>
inline HeapThingHeader *
HeapThingWrapper<T>::headerPointer()
{
    return &header_;
}

template <typename T>
inline const T &
HeapThingWrapper<T>::payload() const
{
    return payload_;
}

template <typename T>
inline T &
HeapThingWrapper<T>::payload()
{
    return payload_;
}

template <typename T>
inline const T *
HeapThingWrapper<T>::payloadPointer() const
{
    return &payload_;
}

template <typename T>
inline T *
HeapThingWrapper<T>::payloadPointer()
{
    return &payload_;
}

//
// UntypedHeapThing
//

template <typename PtrT>
inline PtrT *
UntypedHeapThing::recastThis()
{
    return reinterpret_cast<PtrT *>(this);
}

template <typename PtrT>
inline const PtrT *
UntypedHeapThing::recastThis() const
{
    return reinterpret_cast<const PtrT *>(this);
}


template <typename T>
inline T *
UntypedHeapThing::dataPointer(uint32_t offset)
{
    uint8_t *ptr = recastThis<uint8_t>() + offset;
    WH_ASSERT(IsPtrAligned(ptr, alignof(T)));
    return reinterpret_cast<T *>(ptr);
}

template <typename T>
inline const T *
UntypedHeapThing::dataPointer(uint32_t offset) const
{
    const uint8_t *ptr = recastThis<uint8_t>() + offset;
    WH_ASSERT(IsPtrAligned(ptr, alignof(T)));
    return reinterpret_cast<const T *>(ptr);
}

template <typename T>
inline T &
UntypedHeapThing::dataRef(uint32_t offset)
{
    return *dataPointer<T>(offset);
}

template <typename T>
inline const T &
UntypedHeapThing::dataRef(uint32_t offset) const
{
    return *dataPointer<T>(offset);
}


//
// HeapThing<HeapType HT>
//

template <HeapType HT>
HeapThing<HT>::HeapThing() {};

template <HeapType HT>
HeapThing<HT>::~HeapThing() {};


//
// HeapThingValue<typename T>
//

template <typename T>
inline
HeapThingValue<T>::HeapThingValue()
  : Value(NullValue())
{}

template <typename T>
inline
HeapThingValue<T>::HeapThingValue(T *thing)
  : Value(NativeObjectValue(thing))
{}

template <typename T>
inline
HeapThingValue<T>::HeapThingValue(const Value &val)
  : Value(val)
{
    WH_ASSERT((val.isNativeObject() &&
               val.getAnyNativeObject()->type() == T::Type) ||
              val.isNull());
}

template <typename T>
inline bool
HeapThingValue<T>::hasHeapThing() const
{
    return !this->isNull();
}

template <typename T>
inline T *
HeapThingValue<T>::maybeGetHeapThing() const
{
    return hasHeapThing() ? nullptr : this->getNativeObject<T>();
}

template <typename T>
inline T *
HeapThingValue<T>::getHeapThing() const
{
    return this->getNativeObject<T>();
}

template <typename T>
inline HeapThingValue<T> &
HeapThingValue<T>::operator =(T *thing)
{
    this->Value::operator=(thing ? NullValue() : NativeObjectValue(thing));
    return *this;
}

template <typename T>
inline HeapThingValue<T> &
HeapThingValue<T>::operator =(const Value &val)
{
    WH_ASSERT((val.isNativeObject() &&
               val.getAnyNativeObject()->type() == T::Type) ||
              val.isNull());
    this->Value::operator=(val);
    return *this;
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__HEAP_THING_INLINES_HPP

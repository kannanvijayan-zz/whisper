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
// HeapThing
//

template <typename PtrT>
inline PtrT *
HeapThing::recastThis()
{
    return reinterpret_cast<PtrT *>(this);
}

template <typename PtrT>
inline const PtrT *
HeapThing::recastThis() const
{
    return reinterpret_cast<const PtrT *>(this);
}


template <typename T>
inline T *
HeapThing::dataPointer(uint32_t offset)
{
    uint8_t *ptr = recastThis<uint8_t>() + offset;
    WH_ASSERT(IsPtrAligned(ptr, alignof(T)));
    return reinterpret_cast<T *>(ptr);
}

template <typename T>
inline const T *
HeapThing::dataPointer(uint32_t offset) const
{
    const uint8_t *ptr = recastThis<uint8_t>() + offset;
    WH_ASSERT(IsPtrAligned(ptr, alignof(T)));
    return reinterpret_cast<const T *>(ptr);
}

template <typename T>
inline T &
HeapThing::dataRef(uint32_t offset)
{
    return *dataPointer<T>(offset);
}

template <typename T>
inline const T &
HeapThing::dataRef(uint32_t offset) const
{
    return *dataPointer<T>(offset);
}


//
// HeapThingValue<typename T, bool Null=false>
//

template <typename T>
inline
HeapThingValue<T, false>::HeapThingValue(T *thing)
  : Value(NativeObjectValue(thing))
{}

template <typename T>
inline T *
HeapThingValue<T, false>::getHeapThing() const
{
    return this->getNativeObject<T>();
}

template <typename T>
inline
HeapThingValue<T, false>::operator T *() const
{
    return getHeapThing();
}

template <typename T>
inline T *
HeapThingValue<T, false>::operator ->() const
{
    return getHeapThing();
}

template <typename T>
inline HeapThingValue<T> &
HeapThingValue<T, false>::operator =(T *thing)
{
    WH_ASSERT(thing != nullptr);
    this->Value::operator=(NativeObjectValue(thing));
    return *this;
}


//
// HeapThingValue<typename T, bool Null=true>
//

template <typename T>
inline
HeapThingValue<T, true>::HeapThingValue()
  : Value(NullValue())
{}

template <typename T>
inline
HeapThingValue<T, true>::HeapThingValue(T *thing)
  : Value(thing ? NativeObjectValue(thing) : NullValue())
{}

template <typename T>
inline bool
HeapThingValue<T, true>::hasHeapThing() const
{
    return !this->isNull();
}

template <typename T>
inline T *
HeapThingValue<T, true>::maybeGetHeapThing() const
{
    return hasHeapThing() ? nullptr : this->getNativeObject<T>();
}

template <typename T>
inline T *
HeapThingValue<T, true>::getHeapThing() const
{
    return this->getNativeObject<T>();
}

template <typename T>
inline
HeapThingValue<T, true>::operator T *() const
{
    return maybeGetHeapThing();
}

template <typename T>
inline T *
HeapThingValue<T, true>::operator ->() const
{
    // Arrow notation should only be used on non-null values.
    return getHeapThing();
}

template <typename T>
inline HeapThingValue<T, true> &
HeapThingValue<T, true>::operator =(T *thing)
{
    this->Value::operator=(thing ? NullValue() : NativeObjectValue(thing));
    return *this;
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__HEAP_THING_INLINES_HPP

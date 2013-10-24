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


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__HEAP_THING_INLINES_HPP

#ifndef WHISPER__GC_HPP
#define WHISPER__GC_HPP

#include "slab.hpp"
#include "gc/local.hpp"
#include "gc/handle.hpp"
#include "gc/field.hpp"


namespace Whisper {

//
// Heap<T>
//
// Actual heap wrapper for a given type.  Specializations for this
// type can inherit from HeapHolder to automatically obtain convenience
// methods.
//
template <typename T> class Heap;

//
// Local<T>
//
// Actual stack-root wrapper for a given type.  Specializations for this
// type can inherit from LocalHolder to automatically obtain convenience
// methods.
//
template <typename T> class Local;

//
// Handle<T>
//
// Handle-wrapper for a type.  Specializations can inherit from
// HandleHolder<T> to automatically obtain convenience methods.
//
template <typename T> class Handle;

//
// MutHandle<T>
//
// Mutable-handle-wrapper for a type.  Specializations can inherit from
// MutHandleHolder<T> to automatically obtain convenience methods.
//
template <typename T> class MutHandle;


template <typename T>
class Local<T *> : public LocalHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline Local(T *ptr)
      : LocalHolder<T *>(ptr)
    {}

    inline Local()
      : LocalHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


template <typename T>
class Heap<T *> : public HeapHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline Heap(T *ptr)
      : HeapHolder<T *>(ptr)
    {}

    inline Heap()
      : HeapHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


template <typename T>
class Handle<T *> : public HandleHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline Handle(T *ptr)
      : HandleHolder<T *>(ptr)
    {}

    inline Handle()
      : HandleHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


template <typename T>
class MutHandle<T *> : public MutHandleHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline MutHandle(T *ptr)
      : MutHandleHolder<T *>(ptr)
    {}

    inline MutHandle()
      : MutHandleHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


} // namespace Whisper


#endif // WHISPER__GC_HPP

#ifndef WHISPER__GC_HPP
#define WHISPER__GC_HPP

#include "slab.hpp"
#include "gc/heap.hpp"
#include "gc/local.hpp"
#include "gc/handle.hpp"


namespace Whisper {

//
// Heap<T>
//
// Actual heap wrapper for a given type.  Specializations for this
// type can inherit from GC::HeapHolder to automatically obtain convenience
// methods.
//
template <typename T> class Heap;

//
// Local<T>
//
// Actual stack-root wrapper for a given type.  Specializations for this
// type can inherit from GC::LocalHolder to automatically obtain convenience
// methods.
//
template <typename T> class Local;

//
// Handle<T>
//
// Handle-wrapper for a type.  Specializations can inherit from
// GC::HandleHolder<T> to automatically obtain convenience methods.
//
template <typename T> class Handle;

//
// MutHandle<T>
//
// Mutable-handle-wrapper for a type.  Specializations can inherit from
// GC::MutHandleHolder<T> to automatically obtain convenience methods.
//
template <typename T> class MutHandle;


namespace GC {
// Auto-specialize pointers for these classes.
// Pointers which are stack-rooted or heap-marked must be pointers to
// SlabThings, or types with specializations for SlabThingTraits.

template <typename T>
struct LocalTraits<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");
    static constexpr LocalKind KIND = LocalKind::SlabThingPointer;

    template <typename Scanner>
    void SCAN(Scanner &scanner, T *&ref) {
        if (ref)
            scanner(SlabThingTraits<T>::SLAB_THING(ref), &ref, 0);
    }

    void UPDATE(void *addr, uint32_t discrim, SlabThing *newPtr)
    {
        WH_ASSERT(discrim == 0);
        T **typedAddr = reinterpret_cast<T **>(addr);

        // UPDATE should only be called for values previously registered
        // via SCAN.  Null pointers are not registered, so should never
        // show up at UPDATE.
        WH_ASSERT(*typedAddr);
        *typedAddr = newPtr;
    }
};

} // namespace GC


template <typename T>
class Local<T *> : public GC::LocalHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline Local(T *ptr)
      : GC::LocalHolder<T *>(ptr)
    {}

    inline Local()
      : GC::LocalHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


template <typename T>
class Heap<T *> : public GC::HeapHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline Heap(T *ptr)
      : GC::HeapHolder<T *>(ptr)
    {}

    inline Heap()
      : GC::HeapHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


template <typename T>
class Handle<T *> : public GC::HandleHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline Handle(T *ptr)
      : GC::HandleHolder<T *>(ptr)
    {}

    inline Handle()
      : GC::HandleHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


template <typename T>
class MutHandle<T *> : public GC::MutHandleHolder<T *>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED, "Type is not a SlabThing.");

  public:
    inline MutHandle(T *ptr)
      : GC::MutHandleHolder<T *>(ptr)
    {}

    inline MutHandle()
      : GC::MutHandleHolder<T *>(nullptr)
    {}

    // Forward '->' to underlying type.
    inline T *operator ->() {
        return this->get();
    }
};


} // namespace Whisper


#endif // WHISPER__GC_HPP

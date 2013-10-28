#ifndef WHISPER__ROOTING_INLINES_HPP
#define WHISPER__ROOTING_INLINES_HPP

#include "rooting.hpp"
#include "runtime.hpp"
#include "vm/heap_thing.hpp"
#include <type_traits>

namespace Whisper {

//
// TypedRootBase<typename T>
//

template <typename T>
inline
TypedRootBase<T>::TypedRootBase(ThreadContext *threadContext, RootKind kind,
                                const T &thing)
  : RootBase(threadContext, kind),
    thing_(thing)
{
    postInit();
}

template <typename T>
inline
TypedRootBase<T>::TypedRootBase(ThreadContext *threadContext, RootKind kind)
  : RootBase(threadContext, kind),
    thing_()
{
    postInit();
}

template <typename T>
inline
TypedRootBase<T>::TypedRootBase(RunContext *runContext, RootKind kind,
                             const T &thing)
  : TypedRootBase(runContext->threadContext(), kind, thing)
{}

template <typename T>
inline
TypedRootBase<T>::TypedRootBase(RunContext *runContext, RootKind kind)
  : TypedRootBase(runContext->threadContext(), kind)
{}

template <typename T>
inline const T &
TypedRootBase<T>::get() const
{
    return thing_;
}

template <typename T>
inline T &
TypedRootBase<T>::get()
{
    return thing_;
}

template <typename T>
inline const T *
TypedRootBase<T>::addr() const
{
    return &thing_;
}

template <typename T>
inline T *
TypedRootBase<T>::addr()
{
    return &thing_;
}

template <typename T>
void
TypedRootBase<T>::set(const T &val)
{
    thing_ = val;
}

template <typename T>
inline
TypedRootBase<T>::operator const T &() const
{
    return thing_;
}

template <typename T>
inline
TypedRootBase<T>::operator T &()
{
    return thing_;
}

template <typename T>
inline TypedRootBase<T> &
TypedRootBase<T>::operator =(const T &other)
{
    thing_ = other;
    return *this;
}

//
// PointerRootBase<typename T>
//

template <typename T>
inline
PointerRootBase<T>::PointerRootBase(ThreadContext *threadContext,
                                    RootKind kind, T *ptr)
  : TypedRootBase<T *>(threadContext, kind, ptr)
{}

template <typename T>
inline
PointerRootBase<T>::PointerRootBase(ThreadContext *threadContext,
                                    RootKind kind)
  : TypedRootBase<T *>(threadContext, kind, nullptr)
{}

template <typename T>
inline
PointerRootBase<T>::PointerRootBase(RunContext *runContext, RootKind kind,
                                    T *ptr)
  : TypedRootBase<T *>(runContext->threadContext(), kind, ptr)
{}

template <typename T>
inline
PointerRootBase<T>::PointerRootBase(RunContext *runContext, RootKind kind)
  : TypedRootBase<T *>(runContext->threadContext(), kind, nullptr)
{}

template <typename T>
inline T *
PointerRootBase<T>::operator ->() const
{
    return this->thing_;
}

template <typename T>
inline
PointerRootBase<T>::operator bool() const
{
    return this->thing_ != nullptr;
}

//
// TypedHeapBase<typename T>
//

template <typename T>
inline
TypedHeapBase<T>::TypedHeapBase(const T &ref)
  : val_(ref)
{}

template <typename T>
inline const T &
TypedHeapBase<T>::get() const
{
    return val_;
}

template <typename T>
inline T &
TypedHeapBase<T>::get()
{
    return val_;
}

template <typename T>
inline const T *
TypedHeapBase<T>::addr() const
{
    return &val_;
}

template <typename T>
inline T *
TypedHeapBase<T>::addr()
{
    return &val_;
}

template <typename T>
inline void 
TypedHeapBase<T>::set(const T &t, VM::HeapThing *holder)
{
    // TODO: mark write barriers if needed.
    val_ = t;
}

template <typename T>
inline
TypedHeapBase<T>::operator const T &() const
{
    return get();
}

template <typename T>
inline
TypedHeapBase<T>::operator T &()
{
    return get();
}


//
// PointerHeapBase<typename T>
//

template <typename T>
inline
PointerHeapBase<T>::PointerHeapBase(T *ptr)
  : TypedHeapBase<T *>(ptr)
{}

template <typename T>
inline T *
PointerHeapBase<T>::operator ->() const
{
    return this->get();
}

template <typename T>
inline
PointerHeapBase<T>::operator bool() const
{
    return this->val_ != nullptr;
}


//
// TypedHandleBase<typename T>
//

template <typename T>
inline
TypedHandleBase<T>::TypedHandleBase(TypedRootBase<T> &rootBase)
  : ref_(rootBase.get())
{}

template <typename T>
inline
TypedHandleBase<T>::TypedHandleBase(TypedHeapBase<T> &heapBase)
  : ref_(heapBase.get())
{}

template <typename T>
inline
TypedHandleBase<T>::TypedHandleBase(const T &ref)
  : ref_(ref)
{}

template <typename T>
inline const T &
TypedHandleBase<T>::get() const
{
    return ref_;
}

template <typename T>
inline
TypedHandleBase<T>::operator const T &() const
{
    return get();
}

//
// PointerHandleBase<typename T>
//

template <typename T>
inline
PointerHandleBase<T>::PointerHandleBase(T * const &locn)
  : TypedHandleBase<T *>(locn)
{}

template <typename T>
inline
PointerHandleBase<T>::PointerHandleBase(TypedRootBase<T *> &base)
  : TypedHandleBase<T *>(base)
{}

template <typename T>
inline
PointerHandleBase<T>::PointerHandleBase(TypedHeapBase<T *> &base)
  : TypedHandleBase<T *>(base)
{}

template <typename T>
inline T *
PointerHandleBase<T>::operator ->() const
{
    return this->ref_;
}

template <typename T>
inline
PointerHandleBase<T>::operator bool() const
{
    return this->ref_ != nullptr;
}

//
// TypedMutHandleBase<typename T>
//

template <typename T>
inline
TypedMutHandleBase<T>::TypedMutHandleBase(T *ptr)
  : ref_(*ptr)
{
    WH_ASSERT(ptr != nullptr);
}

template <typename T>
inline T &
TypedMutHandleBase<T>::get() const
{
    return ref_;
}

template <typename T>
inline void
TypedMutHandleBase<T>::set(const T &t) const
{
    ref_ = t;
}

template <typename T>
inline
TypedMutHandleBase<T>::operator T &() const
{
    return get();
}

template <typename T>
inline TypedMutHandleBase<T> &
TypedMutHandleBase<T>::operator =(const T &val)
{
    set(val);
    return *this;
}


//
// PointerMutHandleBase<typename T>
//

template <typename T>
inline
PointerMutHandleBase<T>::PointerMutHandleBase(T **ptr)
  : TypedMutHandleBase<T *>(ptr)
{}

template <typename T>
inline T *
PointerMutHandleBase<T>::operator ->() const
{
    return this->ref_;
}

template <typename T>
inline
PointerMutHandleBase<T>::operator bool() const
{
    return this->ref_ != nullptr;
}

//
// Root<T *>
//

template <typename T>
inline
Root<T *>::Root(RunContext *cx)
  : PointerRootBase<T>(cx, RootKind::HeapThing)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
Root<T *>::Root(ThreadContext *cx)
  : PointerRootBase<T>(cx, RootKind::HeapThing)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
Root<T *>::Root(RunContext *cx, T *ptr)
  : PointerRootBase<T>(cx, RootKind::HeapThing, ptr)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
Root<T *>::Root(ThreadContext *cx, T *ptr)
  : PointerRootBase<T>(cx, RootKind::HeapThing, ptr)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline Root<T *> &
Root<T *>::operator =(T * const &other)
{
    this->TypedRootBase<T *>::operator =(other);
    return *this;
}

//
// Heap<T *>
//

template <typename T>
inline
Heap<T *>::Heap(T *ptr)
  : PointerHeapBase<T>(ptr)
{}


//
// Handle<T *>
//

template <typename T>
inline
Handle<T *>::Handle(T * const &locn)
  : PointerHandleBase<T>(locn)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
Handle<T *>::Handle(const Root<T *> &root)
  : PointerHandleBase<T>(root)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
Handle<T *>::Handle(const Heap<T *> &root)
  : PointerHandleBase<T>(root)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
/*static*/ inline Handle<T *>
Handle<T *>::FromTracedLocation(T * const &locn)
{
    return Handle<T *>(locn);
}


//
// MutHandle<T *>
//

template <typename T>
inline
MutHandle<T *>::MutHandle(T **ptr)
  : PointerMutHandleBase<T>(ptr)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
MutHandle<T *>::MutHandle(Root<T *> *root)
  : PointerMutHandleBase<T>(*root)
{
    static_assert(std::is_base_of<VM::HeapThing, T>::value,
                  "Type is not a heap thing.");
}

template <typename T>
inline
MutHandle<T *>
MutHandle<T *>::FromTracedLocation(T **locn)
{
    return MutHandle<T *>(locn);
}

template <typename T>
inline MutHandle<T *> &
MutHandle<T *>::operator =(const Root<T *> &other)
{
    this->TypedMutHandleBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline MutHandle<T *> &
MutHandle<T *>::operator =(const Handle<T *> &other)
{
    this->TypedMutHandleBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline MutHandle<T *> &
MutHandle<T *>::operator =(const MutHandle<T *> &other)
{
    this->TypedMutHandleBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline MutHandle<T *> &
MutHandle<T *>::operator =(T *other)
{
    this->TypedMutHandleBase<T *>::operator =(other);
    return *this;
}

//
// VectorRootBase<T>
//

template <typename T>
inline
VectorRootBase<T>::VectorRootBase(ThreadContext *threadContext, RootKind kind)
  : RootBase(threadContext, kind),
    things_()
{}

template <typename T>
inline
VectorRootBase<T>::VectorRootBase(RunContext *runContext, RootKind kind)
  : RootBase(runContext->threadContext(), kind),
    things_()
{}

template <typename T>
inline Handle<T>
VectorRootBase<T>::get(uint32_t idx) const
{
    WH_ASSERT(idx < things_.size());
    return Handle<T>::FromTracedLocation(things_[idx]);
}

template <typename T>
inline MutHandle<T>
VectorRootBase<T>::get(uint32_t idx)
{
    WH_ASSERT(idx < things_.size());
    return MutHandle<T>::FromTracedLocation(&things_[idx]);
}

template <typename T>
inline const T &
VectorRootBase<T>::ref(uint32_t idx) const
{
    WH_ASSERT(idx < things_.size());
    return things_[idx];
}

template <typename T>
inline T &
VectorRootBase<T>::ref(uint32_t idx)
{
    WH_ASSERT(idx < things_.size());
    return things_[idx];
}

template <typename T>
inline Handle<T>
VectorRootBase<T>::operator [](uint32_t idx) const
{
    return get(idx);
}

template <typename T>
inline MutHandle<T>
VectorRootBase<T>::operator [](uint32_t idx)
{
    return get(idx);
}

template <typename T>
inline uint32_t
VectorRootBase<T>::size() const
{
    return things_.size();
}

template <typename T>
inline void
VectorRootBase<T>::append(const T &val)
{
    things_.push_back(val);
}


//
// VectorRoot<T *>
//

template <typename T>
inline
VectorRoot<T *>::VectorRoot(RunContext *cx)
  : VectorRootBase<T *>(cx, RootKind::HeapThingVector)
{}

template <typename T>
inline
VectorRoot<T *>::VectorRoot(ThreadContext *cx)
  : VectorRootBase<T *>(cx, RootKind::HeapThingVector)
{}


} // namespace Whisper

#endif // WHISPER__ROOTING_INLINES_HPP

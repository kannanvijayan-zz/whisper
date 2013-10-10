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

template <typename T>
inline TypedRootBase<T> &
TypedRootBase<T>::operator =(const TypedRootBase<T> &other)
{
    thing_ = other.thing_;
    return *this;
}

template <typename T>
inline TypedRootBase<T> &
TypedRootBase<T>::operator =(const TypedHandleBase<T> &other)
{
    thing_ = other.get();
}

template <typename T>
inline TypedRootBase<T> &
TypedRootBase<T>::operator =(const TypedMutableHandleBase<T> &other)
{
    thing_ = other.get();
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
    return this->thing_ == nullptr;
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
PointerHandleBase<T>::PointerHandleBase(TypedRootBase<T *> &base)
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
    return this->ref_ == nullptr;
}

//
// TypedMutableHandleBase<typename T>
//

template <typename T>
inline
TypedMutableHandleBase<T>::TypedMutableHandleBase(T &ref)
  : ref_(ref)
{}

template <typename T>
inline T &
TypedMutableHandleBase<T>::get() const
{
    return ref_;
}

template <typename T>
inline void
TypedMutableHandleBase<T>::set(const T &t) const
{
    ref_ = t;
}

template <typename T>
inline
TypedMutableHandleBase<T>::operator T &() const
{
    return get();
}

template <typename T>
inline TypedMutableHandleBase<T> &
TypedMutableHandleBase<T>::operator =(const TypedRootBase<T> &rootBase)
{
    ref_ = rootBase.get();
}

template <typename T>
inline TypedMutableHandleBase<T> &
TypedMutableHandleBase<T>::operator =(const TypedHandleBase<T> &hBase)
{
    ref_ = set(hBase.get());
}

template <typename T>
inline TypedMutableHandleBase<T> &
TypedMutableHandleBase<T>::operator =(const TypedMutableHandleBase<T> &hBase)
{
    ref_ = set(hBase.get());
}


//
// PointerMutableHandleBase<typename T>
//

template <typename T>
inline
PointerMutableHandleBase<T>::PointerMutableHandleBase(TypedRootBase<T *> &base)
  : TypedMutableHandleBase<T *>(base)
{}

template <typename T>
inline T *
PointerMutableHandleBase<T>::operator ->() const
{
    return this->rootBase_.get();
}

template <typename T>
inline
PointerMutableHandleBase<T>::operator bool() const
{
    return this->rootBase_.get() == nullptr;
}



//
// Root-wrapped Value
//

template <typename T>
inline bool
Root<Value>::isNativeObjectOf() const
{
    return thing_.isNativeObjectOf<T>();
}

template <typename T>
inline T *
Root<Value>::getNativeObject() const
{
    return thing_.getNativeObject<T>();
}

template <typename T>
inline T *
Root<Value>::getForeignObject() const
{
    return thing_.getForeignObject<T>();
}

template <typename CharT>
inline unsigned
Root<Value>::readImmString8(CharT *buf) const
{
    return thing_.readImmString8<CharT>(buf);
}

template <typename CharT>
inline unsigned
Root<Value>::readImmString16(CharT *buf) const
{
    return thing_.readImmString16<CharT>(buf);
}

template <typename CharT>
inline unsigned
Root<Value>::readImmString(CharT *buf) const
{
    return thing_.readImmString<CharT>(buf);
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
Root<T *>::operator =(const Root<T *> &other)
{
    this->TypedRootBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline Root<T *> &
Root<T *>::operator =(const Handle<T *> &other)
{
    this->TypedRootBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline Root<T *> &
Root<T *>::operator =(const MutableHandle<T *> &other)
{
    this->TypedRootBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline Root<T *> &
Root<T *>::operator =(T *other)
{
    this->TypedRootBase<T *>::operator =(other);
    return *this;
}


//
// Handle<T *>
//

template <typename T>
inline
Handle<T *>::Handle(Root<T *> &root)
  : PointerHandleBase<T>(root)
{}


//
// MutableHandle<T *>
//


template <typename T>
inline
MutableHandle<T *>::MutableHandle(Root<T *> &root)
  : PointerMutableHandleBase<T>(root)
{}

template <typename T>
inline MutableHandle<T *> &
MutableHandle<T *>::operator =(const Root<T *> &other)
{
    this->TypedMutableHandleBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline MutableHandle<T *> &
MutableHandle<T *>::operator =(const Handle<T *> &other)
{
    this->TypedMutableHandleBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline MutableHandle<T *> &
MutableHandle<T *>::operator =(const MutableHandle<T *> &other)
{
    this->TypedMutableHandleBase<T *>::operator =(other);
    return *this;
}

template <typename T>
inline MutableHandle<T *> &
MutableHandle<T *>::operator =(T *other)
{
    this->TypedMutableHandleBase<T *>::operator =(other);
    return *this;
}


} // namespace Whisper

#endif // WHISPER__ROOTING_INLINES_HPP

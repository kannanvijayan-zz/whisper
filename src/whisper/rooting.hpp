#ifndef WHISPER__ROOTING_HPP
#define WHISPER__ROOTING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

namespace Whisper {

template <typename T> class TypedHandleBase;
template <typename T> class TypedMutableHandleBase;

//
// RootKind is an enum describing the kind of thing being rooted.
// 
enum class RootKind : uint8_t
{
    INVALID = 0,
    Value,
    Object,
    LIMIT
};

//
// RootBase
//
// Base class for stack-rooted references to things.
//
class RootBase
{
  protected:
    ThreadContext *threadContext_;
    RootBase *next_;
    RootKind kind_;

    inline RootBase(ThreadContext *threadContext, RootKind kind)
      : threadContext_(threadContext),
        next_(threadContext->roots()),
        kind_(kind)
    {}

    inline void postInit() {
        threadContext_->roots_ = this;
    }

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }
    inline RootBase *next() const {
        return next_;
    }
    inline RootKind kind() const {
        return kind_;
    }
};

//
// TypedRootBase<...>
//
// Type-specialized base class for rooted things.
//

template <typename T>
class TypedRootBase : public RootBase
{
  protected:
    T thing_;

  public:
    inline TypedRootBase(ThreadContext *threadContext, RootKind kind,
                         const T &thing)
      : RootBase(threadContext, kind),
        thing_(thing)
    {
        postInit();
    }

    inline TypedRootBase(ThreadContext *threadContext, RootKind kind)
      : RootBase(threadContext, kind),
        thing_()
    {
        postInit();
    }

    inline TypedRootBase(RunContext *runContext, RootKind kind,
                         const T &thing)
      : TypedRootBase(runContext->threadContext(), kind, thing)
    {}

    inline TypedRootBase(RunContext *runContext, RootKind kind)
      : TypedRootBase(runContext->threadContext(), kind)
    {}

    inline const T &get() const {
        return thing_;
    }
    inline T &get() {
        return thing_;
    }

    inline void set(const T &val) {
        thing_ = val;
    }

    inline operator const T &() const {
        return thing_;
    }
    inline operator T &() {
        return thing_;
    }

    inline TypedRootBase<T> &operator =(const T &other) {
        thing_ = other;
        return *this;
    }
    inline TypedRootBase<T> &operator =(const TypedRootBase<T> &other) {
        thing_ = other.thing_;
        return *this;
    }
    inline TypedRootBase<T> &operator =(const TypedHandleBase<T> &other);
    inline TypedRootBase<T> &operator =(const TypedMutableHandleBase<T> &other);
};

template <typename T>
class PointerRootBase : public TypedRootBase<T *>
{
  public:
    inline PointerRootBase(ThreadContext *threadContext, RootKind kind)
      : TypedRootBase<T *>(threadContext, kind, nullptr)
    {}

    inline PointerRootBase(ThreadContext *threadContext, RootKind kind,
                           T *ptr)
      : TypedRootBase<T *>(threadContext, kind, ptr)
    {}

    inline T *operator ->() const {
        return this->thing_;
    }
};

template <typename T>
class Root
{
    Root(const Root<T> &other) = delete;
};


//
// Lightweight handle classes that uses an underlying Root.
//
template <typename T>
class TypedHandleBase
{
  protected:
    TypedRootBase<T> &rootBase_;

  public:
    TypedHandleBase(TypedRootBase<T> &rootBase)
      : rootBase_(rootBase)
    {}

    const T &get() const {
        return rootBase_.get();
    }

    operator const T &() const {
        return get();
    }
};

template <typename T>
class PointerHandleBase : public TypedHandleBase<T *>
{
  public:
    PointerHandleBase(TypedRootBase<T *> &base)
      : TypedHandleBase<T *>(base)
    {}

    inline T *operator ->() const {
        return this->rootBase_.get();
    }
};


//
// Lightweight mutable handle classes that uses an underlying Root.
//
template <typename T>
class TypedMutableHandleBase
{
  protected:
    TypedRootBase<T> &rootBase_;

  public:
    TypedMutableHandleBase(TypedRootBase<T> &rootBase)
      : rootBase_(rootBase)
    {}

    T &get() const {
        return rootBase_.get();
    }

    void set(const T &t) const {
        rootBase_.set(t);
    }

    operator T &() const {
        return get();
    }

    TypedMutableHandleBase<T> &operator =(const TypedRootBase<T> &rootBase) {
        rootBase_.set(rootBase.get());
    }

    TypedMutableHandleBase<T> &operator =(
        const TypedHandleBase<T> &hBase)
    {
        rootBase_.set(hBase.get());
    }

    TypedMutableHandleBase<T> &operator =(
        const TypedMutableHandleBase<T> &hBase)
    {
        rootBase_.set(hBase.get());
    }
};

template <typename T>
class PointerMutableHandleBase : public TypedMutableHandleBase<T *>
{
  public:
    PointerMutableHandleBase(TypedRootBase<T *> &base)
      : TypedMutableHandleBase<T *>(base)
    {}

    inline T *operator ->() const {
        return this->rootBase_.get();
    }
};

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



} // namespace Whisper

#endif // WHISPER__ROOTING_HPP

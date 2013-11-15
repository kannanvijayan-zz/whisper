#ifndef WHISPER__ROOTING_HPP
#define WHISPER__ROOTING_HPP

#include <vector>
#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"

namespace Whisper {

class RunContext;
class ThreadContext;

template <typename T> class TypedHandleBase;
template <typename T> class TypedMutHandleBase;

//
// RootKind is an enum describing the kind of thing being rooted.
// 
enum class RootKind : uint8_t
{
    INVALID = 0,
    Value,
    HeapThing,
    ValueVector,
    HeapThingVector,
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

    RootBase(ThreadContext *threadContext, RootKind kind);

    void postInit();

  public:
    ThreadContext *threadContext() const;

    RootBase *next() const;

    RootKind kind() const;
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

    inline TypedRootBase(ThreadContext *threadContext, RootKind kind,
                         const T &thing);

    inline TypedRootBase(ThreadContext *threadContext, RootKind kind);

    inline TypedRootBase(RunContext *runContext, RootKind kind,
                         const T &thing);

    inline TypedRootBase(RunContext *runContext, RootKind kind);

  public:
    inline const T &get() const;
    inline T &get();

    inline const T *addr() const;
    inline T *addr();

    inline void set(const T &val);

    inline operator const T &() const;
    inline operator T &();

    inline TypedRootBase<T> &operator =(const T &other);
};

template <typename T>
class PointerRootBase : public TypedRootBase<T *>
{
  protected:
    inline PointerRootBase(ThreadContext *threadContext, RootKind kind, T *ptr);
    inline PointerRootBase(ThreadContext *threadContext, RootKind kind);
    inline PointerRootBase(RunContext *runContext, RootKind kind, T *ptr);
    inline PointerRootBase(RunContext *runContext, RootKind kind);

  public:
    inline T *operator ->() const;
    inline explicit operator bool() const;
};

template <typename T>
class Root
{
    Root(const Root<T> &other) = delete;
    Root(Root<T> &&other) = delete;
};

//
// Class that manages a heap value or pointer.
//
template <typename T>
class TypedHeapBase
{
  protected:
    T val_;

    inline TypedHeapBase(const T &ref);

  public:
    inline const T &get() const;
    inline T &get();
    inline const T *addr() const;
    inline T *addr();
    inline void set(const T &t, VM::HeapThing *holder);
    inline operator const T &() const;
    inline operator T &();

    TypedHeapBase<T> &operator=(const TypedHeapBase<T> &ref) = delete;
};

template <typename T>
class PointerHeapBase : public TypedHeapBase<T *>
{
  protected:
    inline PointerHeapBase(T *ptr);

  public:
    inline T *operator ->() const;
    inline explicit operator bool() const;
};

template <typename T>
class Heap
{
    Heap(const Heap<T> &other) = delete;
    Heap(Heap<T> &&other) = delete;
};


//
// Lightweight handle classes that uses an underlying Root.
//
template <typename T>
class TypedHandleBase
{
  protected:
    const T &ref_;

    inline TypedHandleBase(TypedRootBase<T> &rootBase);
    inline TypedHandleBase(TypedHeapBase<T> &rootBase);
    inline TypedHandleBase(const T &ref);

  public:
    inline const T &get() const;
    inline operator const T &() const;
};

template <typename T>
class PointerHandleBase : public TypedHandleBase<T *>
{
  protected:
    inline PointerHandleBase(T * const &locn);
    inline PointerHandleBase(TypedRootBase<T *> &base);
    inline PointerHandleBase(TypedHeapBase<T *> &base);

  public:
    inline T *operator ->() const;
    inline explicit operator bool() const;
};

template <typename T>
class Handle
{
    Handle(const Handle<T> &other) = delete;
    Handle(Handle<T> &&other) = delete;
};


//
// Lightweight mutable handle classes that uses an underlying Root.
//
template <typename T>
class TypedMutHandleBase
{
  protected:
    T &ref_;

    inline TypedMutHandleBase(T *ptr);

  public:
    inline T &get() const;
    inline void set(const T &t);
    inline operator const T &() const;
    inline operator T &();

    inline TypedMutHandleBase<T> &operator =(const T &val);
};

template <typename T>
class PointerMutHandleBase : public TypedMutHandleBase<T *>
{
  protected:
    inline PointerMutHandleBase(T **ptr);

  public:
    inline const T *operator ->() const;
    inline T *operator ->();
    inline explicit operator bool() const;
};

template <typename T>
class MutHandle
{
    MutHandle(const MutHandle<T> &other) = delete;
    MutHandle(MutHandle<T> &&other) = delete;
};


//
// Rooted Value
//

template <>
class Root<Value> : public TypedRootBase<Value>
{
  public:
    Root(RunContext *cx);
    Root(ThreadContext *cx);
    Root(RunContext *cx, const Value &val);
    Root(ThreadContext *cx, const Value &val);

    const Value *operator ->() const;
    Value *operator ->();

    Root<Value> &operator =(const Value &other);
};

//
// Rooted Pointer
//

template <typename T>
class Root<T *> : public PointerRootBase<T>
{
  public:
    inline Root(RunContext *cx);
    inline Root(ThreadContext *cx);
    inline Root(RunContext *cx, T *);
    inline Root(ThreadContext *cx, T *);

    inline Root<T *> &operator =(T * const &other);
};


//
// Heap Value
//

template <>
class Heap<Value> : public TypedHeapBase<Value>
{
  public:
    Heap(const Value &val);

    const Value *operator ->() const;
    Value *operator ->();
};

//
// Heap Pointer
//

template <typename T>
class Heap<T *> : public PointerHeapBase<T>
{
  public:
    inline Heap(T *ptr);
};


//
// Handle Value
//

template <>
class Handle<Value> : public TypedHandleBase<Value>
{
  private:
    Handle(const Value &root);

  public:
    Handle(const Root<Value> &root);
    Handle(const Heap<Value> &heap);
    Handle(const MutHandle<Value> &mut);
    static Handle<Value> FromTracedLocation(const Value &locn);

    const Value *operator ->();
};

//
// Handle Pointer
//

template <typename T>
class Handle<T *> : public PointerHandleBase<T>
{
    inline Handle(T * const &locn);

  public:
    inline Handle(const Root<T *> &root);
    inline Handle(const Heap<T *> &root);
    inline Handle(const MutHandle<T *> &mut);

    static inline Handle<T *> FromTracedLocation(T * const &locn);
};


//
// MutHandle Value
//

template <>
class MutHandle<Value> : public TypedMutHandleBase<Value>
{
  private:
    MutHandle(Value *val);

  public:
    MutHandle(Root<Value> *root);
    MutHandle(Heap<Value> *heap);
    static MutHandle<Value> FromTracedLocation(Value *locn);

    const Value *operator ->() const;
    Value *operator ->();

    MutHandle<Value> &operator =(const Value &other);
};


//
// MutHandle Pointer
//

template <typename T>
class MutHandle<T *> : public PointerMutHandleBase<T>
{
  private:
    inline MutHandle(T **ptr);

  public:
    inline MutHandle(Root<T *> *root);
    static inline MutHandle<T *> FromTracedLocation(T **locn);

    inline MutHandle<T *> &operator =(const Root<T *> &other);
    inline MutHandle<T *> &operator =(const Handle<T *> &other);
    inline MutHandle<T *> &operator =(const MutHandle<T *> &other);
    inline MutHandle<T *> &operator =(T *other);
};


//
// VectorRootBase<...>
//
// Type-specialized base class for vectors of rooted things.
//

template <typename T>
class VectorRootBase : public RootBase
{
  protected:
    std::vector<T> things_;

    inline VectorRootBase(ThreadContext *threadContext, RootKind kind);
    inline VectorRootBase(RunContext *runContext, RootKind kind);

  public:
    inline Handle<T> get(uint32_t idx) const;
    inline MutHandle<T> get(uint32_t idx);

    inline const T &ref(uint32_t idx) const;
    inline T &ref(uint32_t idx);

    inline Handle<T> operator [](uint32_t idx) const;
    inline MutHandle<T> operator [](uint32_t idx);

    inline uint32_t size() const;
    inline void append(const T &val);
};

template <typename T>
class VectorRoot
{
    VectorRoot(const VectorRoot<T> &other) = delete;
    VectorRoot(VectorRoot<T> &&other) = delete;
};

template <>
class VectorRoot<Value> : public VectorRootBase<Value>
{
  public:
    VectorRoot(RunContext *cx);
    VectorRoot(ThreadContext *cx);
};

template <typename T>
class VectorRoot<T *> : public VectorRootBase<T *>
{
  public:
    VectorRoot(RunContext *cx);
    VectorRoot(ThreadContext *cx);
};


} // namespace Whisper

#endif // WHISPER__ROOTING_HPP

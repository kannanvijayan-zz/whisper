#ifndef WHISPER__ROOTING_HPP
#define WHISPER__ROOTING_HPP

#include <vector>
#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

namespace Whisper {

//
// HeapBase
//
// Base class of all traced values kept on the heap.
//
class HeapBase
{
  protected:
    inline HeapBase() {}
    inline HeapBase(const HeapBase &other) {}
};

//
// HandleBase
//
// Base class of all handles to rooted things.
//
class HandleBase
{
  protected:
    inline HandleBase() {}
    inline HandleBase(const HandleBase &other) {}
};

//
// MutHandleBase
//
// Base class of all mutable handles to rooted things.
//
class MutHandleBase
{
  protected:
    inline MutHandleBase() {}
    inline MutHandleBase(const MutHandleBase &other) {}
};

//
// Pre-declaration for various typed rooting helpers.
//

template <typename T> class TypedRootBase;
template <typename T> class TypedHeapBase;
template <typename T> class TypedHandleBase;
template <typename T> class TypedMutHandleBase;


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

    template <typename... Args>
    inline TypedRootBase(ThreadContext *threadContext, RootKind kind,
                         Args... args)
      : RootBase(threadContext, kind),
        thing_(std::forward<Args>(args)...)
    {
        postInit();
    }

    template <typename... Args>
    inline TypedRootBase(RunContext *runContext, RootKind kind,
                         Args... args)
      : TypedRootBase<T>(runContext->threadContext(), kind,
                         std::forward<Args>(args)...)
    {}

  public:
    inline const T &get() const;
    inline T &get();

    inline const T *addr() const;
    inline T *addr();

    inline void set(const T &val);
    inline void set(T &&val);

    inline operator const T &() const;
    inline operator T &();

    inline bool operator == (const T &other) const;
    inline bool operator == (const TypedRootBase<T> &other) const;
    inline bool operator == (const TypedHeapBase<T> &other) const;
    inline bool operator == (const TypedHandleBase<T> &other) const;
    inline bool operator == (const TypedMutHandleBase<T> &other) const;

    inline TypedRootBase<T> &operator =(const T &other);
    inline TypedRootBase<T> &operator =(T &&other);
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
// Typed helper class for HeapBase
//

template <typename T>
class TypedHeapBase : public HeapBase
{
  protected:
    T val_;

    template <typename... Args>
    inline TypedHeapBase(Args... args)
      : HeapBase(),
        val_(std::forward<Args>(args)...)
    {}

  public:
    inline const T &get() const;
    inline T &get();
    inline const T *addr() const;
    inline T *addr();

    template <typename Holder>
    inline void set(const T &t, Holder *holder)
    {
        // TODO: mark write barriers if needed.
        // SlabThing *slabThing = SlabThingTraits<Holder>::SLAB_THING(holder);
        // markSlabThing(slabThing);
        val_ = t;
    }

    inline operator const T &() const;
    inline operator T &();

    inline bool operator == (const T &other) const;
    inline bool operator == (const TypedRootBase<T> &other) const;
    inline bool operator == (const TypedHeapBase<T> &other) const;
    inline bool operator == (const TypedHandleBase<T> &other) const;
    inline bool operator == (const TypedMutHandleBase<T> &other) const;

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
// Lightweight handle class that uses an underlying rooted thing,
// either rooted or on the heap.
//
template <typename T>
class TypedHandleBase : public HandleBase
{
  protected:
    const T &ref_;

    inline TypedHandleBase(TypedRootBase<T> &rootBase);
    inline TypedHandleBase(TypedHeapBase<T> &rootBase);
    inline TypedHandleBase(const T &ref);

  public:
    inline const T &get() const;
    inline operator const T &() const;

    inline bool operator == (const T &other) const;
    inline bool operator == (const TypedRootBase<T> &other) const;
    inline bool operator == (const TypedHeapBase<T> &other) const;
    inline bool operator == (const TypedHandleBase<T> &other) const;
    inline bool operator == (const TypedMutHandleBase<T> &other) const;
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
class TypedMutHandleBase : public MutHandleBase
{
  protected:
    T &ref_;

    inline TypedMutHandleBase(T *ptr);

  public:
    inline T &get() const;
    inline void set(const T &t);
    inline operator const T &() const;
    inline operator T &();

    inline bool operator == (const T &other) const;
    inline bool operator == (const TypedRootBase<T> &other) const;
    inline bool operator == (const TypedHeapBase<T> &other) const;
    inline bool operator == (const TypedHandleBase<T> &other) const;
    inline bool operator == (const TypedMutHandleBase<T> &other) const;

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
// Rooted pointer to a slab allocation.
//

template <typename T>
class Root<T *> : public PointerRootBase<T>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED,
                  "Type has no specialization for SlabThingTraits.");
  public:
    inline Root(RunContext *cx);
    inline Root(ThreadContext *cx);
    inline Root(RunContext *cx, T *);
    inline Root(ThreadContext *cx, T *);

    inline Root<T *> &operator =(T * const &other);
};


//
// Heap Pointer
//

template <typename T>
class Heap<T *> : public PointerHeapBase<T>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED,
                  "Type has no specialization for SlabThingTraits.");
  public:
    inline Heap(T *ptr);
};


//
// Handle Pointer
//

template <typename T>
class Handle<T *> : public PointerHandleBase<T>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED,
                  "Type has no specialization for SlabThingTraits.");
    inline Handle(T * const &locn);

  public:
    inline Handle(const Root<T *> &root);
    inline Handle(const Heap<T *> &root);
    inline Handle(const MutHandle<T *> &mut);

    static inline Handle<T *> FromTracedLocation(T * const &locn);
};


//
// MutHandle Pointer
//

template <typename T>
class MutHandle<T *> : public PointerMutHandleBase<T>
{
    static_assert(SlabThingTraits<T>::SPECIALIZED,
                  "Type has no specialization for SlabThingTraits.");
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


/*

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

template <typename T>
class VectorRoot<T *> : public VectorRootBase<T *>
{
  public:
    VectorRoot(RunContext *cx);
    VectorRoot(ThreadContext *cx);
};

*/


} // namespace Whisper

#endif // WHISPER__ROOTING_HPP

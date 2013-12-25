#ifndef WHISPER__GC__ROOTS_HPP
#define WHISPER__GC__ROOTS_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace GC {

class ThreadContext;

//
// RootKind
//
// An enum describing the kind of thing being rooted.  Every rootable
// type must provide a specialization of |RootTraits| that allows
// extraction of a RootKind from it.
//

enum class RootKind : uint32_t
{
    // A rooted pointer to a slab thing.
    SlabThingPointer
};

//
// RootTraits<T>()
//
// Specializations of RootKindTrait must be provided by every rooted
// type.  The specialization must contain the following definitions:
//
//  static constexpr RootKind KIND;
//      The RootKind for the type being rooted.
//
//  template <typename Scanner> void SCAN(Scanner &scanner);
//      Method to scan the rooted thing for references.
//      For each heap reference contained within the rooted thing,
//      the method |scanner(ptr, addr, discrim)| should be called
//      once, where |ptr| is pointer (of type SlabThing*), |addr|
//      is the address of the pointer (of type void*), and |discrim|
//      is a T-specific uint32_t value that helps distinguish the
//      format of the reference being held.
//
//  void UPDATE(void *addr, uint32_t discrim, SlabThing *newPtr);
//      Method to update a previously-scanned pointer address with
//      a relocated pointer.
//
template <typename T> struct RootTraits;

//
// RootTypeTrait<T>()
//
// Specializations of RootTypeTrait must be provided for every RootKind.
// The specialization must define a type TYPE, which is the unchecked
// type for the kind.
//
template <RootKind KIND> struct RootTypeTrait;

//
// RootHolderBase
//
// Untyped base class for root holders.
//

class RootHolderBase
{
  protected:
    ThreadContext *threadContext_;
    RootHolderBase *next_;
    RootKind kind_;

    inline RootHolderBase(ThreadContext *threadContext, RootKind kind);

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline RootHolderBase *next() const {
        return next_;
    }

    inline const RootKind &kind() const {
        return kind_;
    }
};


//
// RootHolderUnchecked
//
// Unchecked typed holder class that refers to a stack-allocated rooted
// thing.
//

template <typename T>
class RootHolderUnchecked : public RootHolderBase
{
  protected:
    T thing_;

    template <typename... Args>
    inline RootHolderUnchecked(ThreadContext *threadContext, RootKind kind,
                               Args... args)
      : RootHolderBase(threadContext, kind),
        thing_(std::forward<Args>(args)...)
    {}
};

//
// RootHolder
//
// Checked holder class that refers to a stack-allocated rooted
// thing.
//

template <typename T>
using UncheckedRootType = typename RootTypeTrait<RootTraits<T>::KIND>::TYPE;

template <typename T>
class RootHolder : public RootHolderUnchecked<UncheckedRootType<T>>
{
    typedef UncheckedRootType<T> UNCHECKED_T;
    typedef RootHolderUnchecked<UNCHECKED_T> BASE;

    static_assert(sizeof(T) == sizeof(UncheckedRootType<T>),
                  "Size mismatch between rooted type and its unchecked "
                  "surrogate.");
    static_assert(alignof(T) == alignof(UncheckedRootType<T>),
                  "Alignment mismatch between rooted type and its unchecked "
                  "surrogate.");

  public:
    template <typename... Args>
    inline RootHolder(ThreadContext *threadContext, Args... args)
      : BASE(threadContext, RootTraits<T>::KIND, args...)
    {}

    inline const T &get() const {
        return reinterpret_cast<const T &>(this->thing_);
    }

    inline T &get() {
        return reinterpret_cast<T &>(this->thing_);
    }
    inline void set(const T &ref) {
        this->thing_ = ref;
    }
    inline void set(T &&ref) {
        this->thing_ = ref;
    }

    inline operator const T &() const {
        return this->get();
    }
    inline operator T &() {
        return this->get();
    }

    inline const T *operator &() const {
        return &(this->get());
    }
    inline T *operator &() {
        return &(this->get());
    }

    inline T &operator =(const T &ref) {
        this->set(ref);
        return this->get();
    }
    inline T &operator =(T &&ref) {
        this->set(ref);
        return this->get();
    }
};

//
// Root<T>
//
// Actual root wrapper for a given type.  Specializations for this
// type can inherit from RootHolder to automatically obtain convenience
// methods.
//
template <typename T> class Root;



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__ROOTS_HPP

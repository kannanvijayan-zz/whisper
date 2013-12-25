#ifndef WHISPER__GC_ROOTS_HPP
#define WHISPER__GC_ROOTS_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace GC {

class ThreadContext;

//
// RootKind
//
// An enum describing the kind of thing being rooted.  Every rootable
// type must provide a specialization of |RootKindTraits| that allows
// extraction of a RootKind from it.
//

enum class RootKind : uint32_t
{
    // A rooted pointer to a slab thing.
    SlabThingPointer
};

//
// RootKindTrait<T>()
//
// Specializations of RootKindTrait must be provided by every rooted
// type.  The specialization must return a RootKind for the type.
//
template <typename T>
inline constexpr RootKind RootKindTrait();

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
using UncheckedRootType = typename RootTypeTrait<RootKindTrait<T>()>::TYPE;

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
      : BASE(threadContext, RootKindTrait<T>(), args...)
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

    inline T &operator =(const T &ref) {
        this->set(ref);
        return this->get();
    }
    inline T &operator =(T &&ref) {
        this->set(ref);
        return this->get();
    }
};


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC_ROOTS_HPP

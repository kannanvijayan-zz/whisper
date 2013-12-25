#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace GC {

class ThreadContext;

//
// LocalKind
//
// An enum describing the kind of thing being rooted on the stack.
// Every stack-rootable type must provide a specialization of
// |LocalTraits| that allows extraction of a LocalKind from it.
//

enum class LocalKind : uint32_t
{
    // A pointer to a slab thing.
    SlabThingPointer
};

//
// LocalTraits<T>()
//
// Specializations of LocalTrait must be provided by every stack-rooted
// type.  The specialization must contain the following definitions:
//
//  static constexpr LocalKind KIND;
//      The LocalKind for the type being stack-rooted.
//
//  template <typename Scanner> void SCAN(Scanner &scanner, T &ref);
//      Method to scan the stack-rooted thing for references.
//      For each heap reference contained within the stack-rooted thing,
//      referenced by |ref|, the method |scanner(ptr, addr, discrim)|
//      should be called once, where |ptr| is pointer (of type SlabThing*),
//      |addr| is the address of the pointer (of type void*), and |discrim|
//      is a T-specific uint32_t value that helps distinguish the
//      format of the reference being held.
//
//  void UPDATE(void *addr, uint32_t discrim, SlabThing *newPtr);
//      Method to update a previously-scanned pointer address with
//      a relocated pointer.
//
template <typename T> struct LocalTraits;

//
// LocalTypeTrait<T>()
//
// Specializations of LocalTypeTrait must be provided for every LocalKind.
// The specialization must define a type TYPE, which is the unchecked
// type for the kind.
//
template <LocalKind KIND> struct LocalTypeTrait;

//
// LocalHolderBase
//
// Untyped base class for stack-root holders.
//

class LocalHolderBase
{
  protected:
    ThreadContext *threadContext_;
    LocalHolderBase *next_;
    LocalKind kind_;

    inline LocalHolderBase(ThreadContext *threadContext, LocalKind kind);

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline LocalHolderBase *next() const {
        return next_;
    }

    inline const LocalKind &kind() const {
        return kind_;
    }
};


//
// LocalHolderUnchecked
//
// Unchecked typed holder class that refers to a stack-rooted
// thing.
//

template <typename T>
class LocalHolderUnchecked : public LocalHolderBase
{
  protected:
    T val_;

    template <typename... Args>
    inline LocalHolderUnchecked(ThreadContext *threadContext, LocalKind kind,
                                Args... args)
      : LocalHolderBase(threadContext, kind),
        val_(std::forward<Args>(args)...)
    {}
};

//
// LocalHolder
//
// Checked holder class that refers to a stack-rooted
// thing.
//

template <typename T>
using UncheckedLocalType = typename LocalTypeTrait<LocalTraits<T>::KIND>::TYPE;

template <typename T>
class LocalHolder : public LocalHolderUnchecked<UncheckedLocalType<T>>
{
    typedef UncheckedLocalType<T> UNCHECKED_T;
    typedef LocalHolderUnchecked<UNCHECKED_T> BASE;

    static_assert(sizeof(T) == sizeof(UncheckedLocalType<T>),
                  "Size mismatch between stack-rooted type and its "
                  "unchecked surrogate.");
    static_assert(alignof(T) == alignof(UncheckedLocalType<T>),
                  "Alignment mismatch between stack-rooted type and its "
                  "unchecked surrogate.");

  public:
    template <typename... Args>
    inline LocalHolder(ThreadContext *threadContext, Args... args)
      : BASE(threadContext, LocalTraits<T>::KIND,
             std::forward<Args>(args)...)
    {}

    inline const T &get() const {
        return reinterpret_cast<const T &>(this->val_);
    }
    inline T &get() {
        return reinterpret_cast<T &>(this->val_);
    }

    inline void set(const T &ref) {
        this->val_ = ref;
    }
    inline void set(T &&ref) {
        this->val_ = ref;
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



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

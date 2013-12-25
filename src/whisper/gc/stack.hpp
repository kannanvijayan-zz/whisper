#ifndef WHISPER__GC__STACK_HPP
#define WHISPER__GC__STACK_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace GC {

class ThreadContext;

//
// StackKind
//
// An enum describing the kind of thing being rooted on the stack.
// Every stack-rootable type must provide a specialization of
// |StackTraits| that allows extraction of a StackKind from it.
//

enum class StackKind : uint32_t
{
    // A pointer to a slab thing.
    SlabThingPointer
};

//
// StackTraits<T>()
//
// Specializations of StackTrait must be provided by every stack-rooted
// type.  The specialization must contain the following definitions:
//
//  static constexpr StackKind KIND;
//      The StackKind for the type being stack-rooted.
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
template <typename T> struct StackTraits;

//
// StackTypeTrait<T>()
//
// Specializations of StackTypeTrait must be provided for every StackKind.
// The specialization must define a type TYPE, which is the unchecked
// type for the kind.
//
template <StackKind KIND> struct StackTypeTrait;

//
// StackHolderBase
//
// Untyped base class for stack-root holders.
//

class StackHolderBase
{
  protected:
    ThreadContext *threadContext_;
    StackHolderBase *next_;
    StackKind kind_;

    inline StackHolderBase(ThreadContext *threadContext, StackKind kind);

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline StackHolderBase *next() const {
        return next_;
    }

    inline const StackKind &kind() const {
        return kind_;
    }
};


//
// StackHolderUnchecked
//
// Unchecked typed holder class that refers to a stack-rooted
// thing.
//

template <typename T>
class StackHolderUnchecked : public StackHolderBase
{
  protected:
    T val_;

    template <typename... Args>
    inline StackHolderUnchecked(ThreadContext *threadContext, StackKind kind,
                                Args... args)
      : StackHolderBase(threadContext, kind),
        val_(std::forward<Args>(args)...)
    {}
};

//
// StackHolder
//
// Checked holder class that refers to a stack-rooted
// thing.
//

template <typename T>
using UncheckedStackType = typename StackTypeTrait<StackTraits<T>::KIND>::TYPE;

template <typename T>
class StackHolder : public StackHolderUnchecked<UncheckedStackType<T>>
{
    typedef UncheckedStackType<T> UNCHECKED_T;
    typedef StackHolderUnchecked<UNCHECKED_T> BASE;

    static_assert(sizeof(T) == sizeof(UncheckedStackType<T>),
                  "Size mismatch between stack-rooted type and its "
                  "unchecked surrogate.");
    static_assert(alignof(T) == alignof(UncheckedStackType<T>),
                  "Alignment mismatch between stack-rooted type and its "
                  "unchecked surrogate.");

  public:
    template <typename... Args>
    inline StackHolder(ThreadContext *threadContext, Args... args)
      : BASE(threadContext, StackTraits<T>::KIND,
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

#endif // WHISPER__GC__STACK_HPP

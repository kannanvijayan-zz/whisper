#ifndef WHISPER__GC__HANDLE_HPP
#define WHISPER__GC__HANDLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/local.hpp"

namespace Whisper {


//
// MutHandles are lightweight pointers to Local<> instances.  They
// allow for mutation of the stored value (i.e. assignment).
//

template <typename T>
class MutHandle
{
    static_assert(GC::StackTraits<T>::Specialized,
                  "GC::StackTraits<T> not specialized.");
    typedef typename GC::DerefTraits<T>::Type DerefType;

  private:
    volatile T * const valAddr_;

    inline MutHandle(T *valAddr)
      : valAddr_(valAddr)
    {}

  public:
    inline MutHandle(const Local<T> &stackVal)
      : valAddr_(stackVal.address())
    {}

    inline static MutHandle FromTrackedPointer(T *valAddr)
    {
        return MutHandle(valAddr);
    }

    inline const T &get() const {
        return *valAddr_;
    }
    inline T &get() {
        return *valAddr_;
    }

    inline void set(const T &other) {
        *valAddr_ = other;
    }
    inline void set(T &&other) {
        *valAddr_ = other;
    }

    inline T *address() const {
        return valAddr_;
    }

    inline operator const T &() const {
        return this->get();
    }
    inline operator T &() {
        return this->get();
    }

    inline T *operator &() const {
        return address();
    }

    inline DerefType *operator ->() const {
        return GC::DerefTraits<T>::Deref(*valAddr_);
    }

    const T &operator =(const T &other) {
        WH_ASSERT(valAddr_ != nullptr);
        *valAddr_ = other;
        return other;
    }
};


//
// Handles are const pointers to values rooted on the stack with
// Local.
//

template <typename T>
class Handle
{
    static_assert(GC::StackTraits<T>::Specialized,
                  "GC::StackTraits<T> not specialized.");

    typedef typename GC::DerefTraits<T>::Type DerefType;

  private:
    volatile const T * const valAddr_;

    inline Handle(const T *valAddr)
      : valAddr_(valAddr)
    {}

  public:
    inline Handle(const Local<T> &stackVal)
      : valAddr_(stackVal.address())
    {}

    inline Handle(const MutHandle<T> &mutHandle)
      : valAddr_(mutHandle.address())
    {}

    inline static Handle FromTrackedPointer(const T *valAddr)
    {
        return Handle(valAddr);
    }

    inline const T &get() const {
        return *valAddr_;
    }

    inline const T *address() const {
        return valAddr_;
    }

    inline operator const T &() const {
        return get();
    }

    inline const T *operator &() const {
        return address();
    }

    inline const DerefType *operator ->() const {
        return GC::DerefTraits<T>::Deref(*valAddr_);
    }

    const Handle<T> &operator =(const Handle<T> &other) = delete;
};


} // namespace Whisper

#endif // WHISPER__GC__HANDLE_HPP

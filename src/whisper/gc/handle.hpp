#ifndef WHISPER__GC__HANDLE_HPP
#define WHISPER__GC__HANDLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/local.hpp"

namespace Whisper {


//
// MutHandles are lightweight mutable pointers to structures which have
// stack lifetime.
//

template <typename T>
class MutHandle
{
    static_assert(AllocTraits<T>::SPECIALIZED,
                  "AllocTraits<T> not specialized.");

  private:
    volatile T *valAddr_;

  public:
    inline MutHandle(const Local<T> &stackVal)
      : valAddr_(&stackVal)
    {}

    inline const T &get() const {
        return *valAddr_;
    }
    inline T &get() {
        return *valAddr_;
    }

    inline operator const T &() const {
        return this->get();
    }
    inline operator T &() {
        return this->get();
    }

    inline void set(const T &other) {
        *valAddr_ = other;
    }

    inline void set(T &&other) {
        *valAddr_ = other;
    }

    inline const T *operator &() const {
        return valAddr_;
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
    static_assert(AllocTraits<T>::SPECIALIZED,
                  "AllocTraits<T> not specialized.");

  private:
    volatile const T *valAddr_;

  public:
    inline Handle(const Local<T> &stackVal)
      : valAddr_(&stackVal)
    {}

    inline Handle(const MutHandle<T> &mutHandle)
      : valAddr_(&mutHandle)
    {}

    inline const T &get() const {
        return *valAddr_;
    }

    inline operator const T &() const {
        return this->get();
    }

    inline const T *operator &() const {
        return valAddr_;
    }

    const Handle<T> &operator =(const Handle<T> &other) = delete;
};


} // namespace Whisper

#endif // WHISPER__GC__HANDLE_HPP

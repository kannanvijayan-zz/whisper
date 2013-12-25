#ifndef WHISPER__GC__HANDLE_HPP
#define WHISPER__GC__HANDLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/local.hpp"
#include "gc/heap.hpp"

namespace Whisper {
namespace GC {


//
// Handles are const pointers to stack-rooted or heap-marked things which
// are already managed.
//

template <typename T>
class HandleHolder
{
  private:
    const T *valAddr_;

  public:
    inline HandleHolder(const LocalHolder<T> &stackVal)
      : valAddr_(&stackVal)
    {}
    inline HandleHolder(const HeapHolder<T> &heapVal)
      : valAddr_(&heapVal)
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

    T &operator =(const HeapHolder<T> &other) = delete;
};


//
// MutHandles are mutable pointers to stack-rooted or heap-marked things which
// are already managed.
//

template <typename T>
class MutHandleHolder
{
  private:
    T *valAddr_;

  public:
    inline MutHandleHolder(const LocalHolder<T> &stackVal)
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

    inline const T *operator &() const {
        return valAddr_;
    }
    inline T *operator &() {
        return valAddr_;
    }

    T &operator =(const HeapHolder<T> &other) = delete;
};



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__HANDLE_HPP

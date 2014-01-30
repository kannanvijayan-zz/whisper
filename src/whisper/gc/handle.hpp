#ifndef WHISPER__GC__HANDLE_HPP
#define WHISPER__GC__HANDLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/local.hpp"
#include "gc/heap.hpp"

namespace Whisper {


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

    inline void set(T &&other) {
        *valAddr_ = other;
    }

    inline const T *operator &() const {
        return valAddr_;
    }

    // Assignments are ok.
    T &operator =(const HeapHolder<T> &other) = delete;
};

//
// MutHeapHandles are mutable handles to things on heap.  To allow
// update, these have to keep a pointer to the containing object.
//
template <typename T>
class MutHeapHandleHolder
{
  private:
    SlabThing *container_;
    T *valAddr_;

  public:
    inline MutHeapHandleHolder(SlabThing *container, const HeapHolder<T> &val)
      : container_(container),
        valAddr_(&val)
    {
        WH_ASSERT(container);
    }

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
    // Cannot convert to simple mutable pointer.
    // Mutations must occur through overloaded
    // operator.
    T *operator &() = delete;

    inline T &operator =(const T &other) {
        *valAddr_ = other;
        // TODO: Kick post-write barriers on container_ here.
    }

    inline T &operator =(T &&other) {
        *valAddr_ = other;
        // TODO: Kick post-write barriers on container_ here.
    }
};



} // namespace Whisper

#endif // WHISPER__GC__HANDLE_HPP

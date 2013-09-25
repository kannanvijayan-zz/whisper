#ifndef WHISPER__RUNTIME__ROOTING_HPP
#define WHISPER__RUNTIME__ROOTING_HPP

#include <type_traits>

#include "common.hpp"
#include "debug.hpp"

#include "runtime/heap_thing.hpp"

namespace Whisper {
namespace Runtime {


class Runtime;


template <typename T>
class HeapPtr
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");
  template <typename X> friend class HandlePtr;
  template <typename X> friend class MutHandlePtr;

  private:
    T *ptr_;

  public:
    inline HeapPtr() : ptr_(nullptr) {}
    inline HeapPtr(T *ptr) : ptr_(ptr) {}

    inline bool isValid() const {
        return ptr_ != nullptr;
    }

    inline explicit operator bool() const {
        return isValid();
    }

    inline T *maybeGet() const {
        return ptr_;
    }

    inline T *get() const {
        WH_ASSERT(isValid());
        return ptr_;
    }

    inline T &operator *() const {
        WH_ASSERT(isValid());
        return *ptr_;
    }

    inline T *operator ->() const {
        WH_ASSERT(isValid());
        return ptr_;
    }

    inline operator T *() const {
        WH_ASSERT(isValid());
        return ptr_;
    }

    inline HeapPtr<T> &operator =(T *ptr) {
        ptr_ = ptr;
        return *this;
    }
};


template <typename T>
class HandlePtr
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");
  private:
    T * const *addr_;

  public:
    inline HandlePtr(T *const&val) : addr_(&val) {}
    inline HandlePtr(HeapPtr<T> heapPtr) : addr_(&heapPtr.ptr_) {}

    inline bool isValid() const {
        return *addr_ != nullptr;
    }

    inline explicit operator bool() const {
        return isValid();
    }

    inline T *maybeGet() const {
        return *addr_;
    }

    inline T *get() const {
        WH_ASSERT(isValid());
        return *addr_;
    }

    inline T &operator *() const {
        WH_ASSERT(isValid());
        return **addr_;
    }

    inline T *operator ->() const {
        WH_ASSERT(isValid());
        return *addr_;
    }

    inline operator T *() const {
        WH_ASSERT(isValid());
        return *addr_;
    }
};

template <typename T>
class MutHandlePtr
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");
  private:
    T **addr_;

  public:
    inline MutHandlePtr(T *&val) : addr_(&val) {}
    inline MutHandlePtr(HeapPtr<T> heapPtr) : addr_(&heapPtr.ptr_) {}

    inline bool isValid() const {
        return *addr_ != nullptr;
    }

    inline explicit operator bool() const {
        return isValid();
    }

    inline T *maybeGet() const {
        return *addr_;
    }

    inline T *get() const {
        WH_ASSERT(isValid());
        return *addr_;
    }

    inline T &operator *() const {
        WH_ASSERT(isValid());
        return **addr_;
    }

    inline T *operator ->() const {
        WH_ASSERT(isValid());
        return *addr_;
    }

    inline operator T *() const {
        WH_ASSERT(isValid());
        return *addr_;
    }

    inline MutHandlePtr<T> &operator =(T *ptr) {
        *addr_ = ptr;
        return *this;
    }
};


template <typename T>
class HeapPtrArray
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");
  template <typename X> friend class HandlePtrArray;
  template <typename X> friend class MutHandlePtrArray;

  private:
    T *ptrs_[0];

  public:
    inline HeapPtrArray(uint32_t length) {
        for (uint32_t i = 0; i < length; i++)
            ptrs_[i] = nullptr;
    }
    inline HeapPtrArray(uint32_t length, T *const *ptrs) {
        for (uint32_t i = 0; i < length; i++)
            ptrs_[i] = ptrs[i];
    }
    inline HeapPtrArray(uint32_t length, T *ptr) {
        for (uint32_t i = 0; i < length; i++)
            ptrs_[i] = ptr;
    }

    inline T *const *ptrs() const {
        return &ptrs_[0];
    }

    inline HandlePtr<T> operator[](uint32_t idx) const {
        return HandlePtr<T>(ptrs_[idx]);
    }

    inline MutHandlePtr<T> operator [](uint32_t idx) {
        return MutHandlePtr<T>(ptrs_[idx]);
    }
};


template <typename T>
class HandlePtrArray
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");
  private:
    uint32_t size_;
    T *const *addr_;

  public:
    inline HandlePtrArray(uint32_t size, T *const *addr)
      : size_(size), addr_(addr) {}

    inline HandlePtrArray(uint32_t size, HeapPtrArray<T> heapPtrArray)
      : size_(size), addr_(heapPtrArray.ptrs_) {}

    inline uint32_t size() const {
        return size_;
    }

    inline T *const *ptrs() const {
        return &addr_[0];
    }

    inline operator T *const *() const {
        return ptrs();
    }

    inline HandlePtr<T> operator[](uint32_t idx) const {
        return HandlePtr<T>(addr_[idx]);
    }
};


template <typename T>
class MutHandlePtrArray
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");
  private:
    uint32_t size_;
    T **addr_;

  public:
    inline MutHandlePtrArray(uint32_t size, T **addr)
      : size_(size), addr_(addr) {}

    inline MutHandlePtrArray(uint32_t size, HeapPtrArray<T> heapPtrArray)
      : size_(size), addr_(heapPtrArray.ptrs_) {}

    inline uint32_t size() const {
        return size_;
    }

    inline T **ptrs() const {
        return &addr_[0];
    }

    inline operator T **() const {
        return ptrs();
    }

    inline MutHandlePtr<T> operator [](uint32_t idx) {
        return MutHandlePtr<T>(addr_[idx]);
    }
};


} // namespace Runtime
} // namespace Whisper

#endif // WHISPER__RUNTIME__ROOTING_HPP

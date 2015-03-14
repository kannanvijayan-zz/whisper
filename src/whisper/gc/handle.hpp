#ifndef WHISPER__GC__HANDLE_HPP
#define WHISPER__GC__HANDLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/local.hpp"

namespace Whisper {


//
// MutHandles are lightweight pointers to rooted values.  They
// allow for mutation of the stored value (i.e. assignment).
//
template <typename T>
class MutHandle
{
    typedef typename DerefTraits<T>::Type DerefType;

  private:
    T * const valAddr_;

    inline MutHandle(T *valAddr)
      : valAddr_(valAddr)
    {}

  public:
    inline MutHandle(Local<T> &stackVal)
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
        *valAddr_ = std::move(other);
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
        return DerefTraits<T>::Deref(*valAddr_);
    }

    const T &operator =(const T &other) {
        WH_ASSERT(valAddr_ != nullptr);
        *valAddr_ = other;
        return other;
    }
    const T &operator =(T &&other) {
        WH_ASSERT(valAddr_ != nullptr);
        *valAddr_ = std::move(other);
        return other;
    }
};

template <typename T>
class MutArrayHandle
{
    typedef typename DerefTraits<T>::Type DerefType;

  private:
    T * const valAddr_;
    uint32_t length_;

    inline MutArrayHandle(T *valAddr, uint32_t length)
      : valAddr_(valAddr),
        length_(length)
    {}

  public:
    inline MutArrayHandle(Local<T> &stackVal)
      : MutArrayHandle(stackVal.address(), 1)
    {}

    inline static MutArrayHandle<T> FromTrackedPointer(T *valAddr,
                                                       uint32_t length)
    {
        return MutArrayHandle<T>(valAddr, length);
    }

    inline const T &get(uint32_t i) const {
        WH_ASSERT(i < length_);
        return valAddr_[i];
    }
    inline T &get(uint32_t i) {
        WH_ASSERT(i < length_);
        return valAddr_[i];
    }

    inline void set(uint32_t i, const T &other) {
        WH_ASSERT(i < length_);
        valAddr_[i] = other;
    }
    inline void set(uint32_t i, T &&other) {
        WH_ASSERT(i < length_);
        valAddr_[i] = other;
    }

    inline const T &operator [](uint32_t idx) const {
        return get(idx);
    }
    inline T &operator [](uint32_t idx) {
        return get(idx);
    }
};


//
// Handles are const pointers to values rooted on the stack with
// Local.
//
template <typename T>
class Handle
{
    typedef typename DerefTraits<T>::Type DerefType;

  private:
    T * const valAddr_;

    inline Handle(T *valAddr)
      : valAddr_(valAddr)
    {}

  public:
    inline Handle(Local<T> &stackVal)
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

    inline DerefType *operator ->() const {
        return DerefTraits<T>::Deref(*valAddr_);
    }

    const Handle<T> &operator =(const Handle<T> &other) = delete;
};

template <typename T>
class ArrayHandle
{
    typedef typename DerefTraits<T>::Type DerefType;

  private:
    T * const valAddr_;
    uint32_t length_;

    inline ArrayHandle(T *valAddr, uint32_t length)
      : valAddr_(valAddr),
        length_(length)
    {}

  public:
    inline ArrayHandle(Local<T> &stackVal)
      : ArrayHandle(stackVal.address(), 1)
    {}

    inline ArrayHandle(const MutArrayHandle<T> &mutHandle)
      : ArrayHandle(mutHandle.valAddr_, mutHandle.length)
    {}

    inline static ArrayHandle<T> FromTrackedPointer(T *valAddr,
                                                    uint32_t length)
    {
        return ArrayHandle<T>(valAddr, length);
    }

    inline const T &get(uint32_t i) const {
        WH_ASSERT(i < length_);
        return valAddr_[i];
    }
    inline T get(uint32_t i) {
        WH_ASSERT(i < length_);
        return valAddr_[i];
    }

    inline const T &operator [](uint32_t idx) const {
        return get(idx);
    }
    inline T operator [](uint32_t idx) {
        return get(idx);
    }
};


} // namespace Whisper

#endif // WHISPER__GC__HANDLE_HPP

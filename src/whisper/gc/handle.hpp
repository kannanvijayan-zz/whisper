#ifndef WHISPER__GC__HANDLE_HPP
#define WHISPER__GC__HANDLE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/local.hpp"
#include "gc/field.hpp"

namespace Whisper {


template <typename T> class MutHandle;
template <typename T> class MutArrayHandle;
template <typename T> class Handle;
template <typename T> class ArrayHandle;

//
// MutHandles are lightweight pointers to rooted values.  They
// allow for mutation of the stored value (i.e. assignment).
//
template <typename T>
class MutHandle
{
    template <typename U> friend class MutArrayHandle;

    typedef typename DerefTraits<T>::Type DerefType;

  private:
    T* const valAddr_;

    inline MutHandle(T* valAddr)
      : valAddr_(valAddr)
    {}

  public:
    inline MutHandle(MutHandle<T> const& other)
      : valAddr_(other.valAddr_)
    {}
    inline MutHandle(MutHandle<T>&& other)
      : valAddr_(other.valAddr_)
    {}
    inline MutHandle(Local<T>* stackVal)
      : valAddr_(stackVal->address())
    {}
    inline MutHandle(LocalArray<T>* stackArray, uint32_t idx)
      : valAddr_(stackArray->address(idx))
    {}
    inline MutHandle(StackField<T>* stackField)
      : valAddr_(stackField->address())
    {}

    inline static MutHandle FromTrackedLocation(T* valAddr)
    {
        return MutHandle(valAddr);
    }

    inline T const& get() const {
        return *valAddr_;
    }
    inline T& get() {
        return *valAddr_;
    }

    inline void set(T const& other) {
        *valAddr_ = other;
    }
    inline void set(T&& other) {
        *valAddr_ = std::move(other);
    }
    OkResult setResult(Result<T> const& result) {
        if (result.isError())
            return ErrorVal();
        *valAddr_ = result.value();
        return OkVal();
    }

    inline T* address() const {
        return valAddr_;
    }

    inline operator T const&() const {
        return this->get();
    }
    inline operator T&() {
        return this->get();
    }

    inline T* operator &() const {
        return address();
    }

    inline DerefType* operator ->() const {
        return DerefTraits<T>::Deref(*valAddr_);
    }

    T const& operator =(T const& other) {
        WH_ASSERT(valAddr_ != nullptr);
        *valAddr_ = other;
        return other;
    }
    T& operator =(T&& other) {
        WH_ASSERT(valAddr_ != nullptr);
        *valAddr_ = std::move(other);
        return other;
    }
};

template <typename T>
class MutArrayHandle
{
  private:
    T* const valAddr_;
    uint32_t length_;

    inline MutArrayHandle(T* valAddr, uint32_t length)
      : valAddr_(valAddr),
        length_(length)
    {}

  public:
    inline MutArrayHandle(Local<T>& stackVal)
      : MutArrayHandle(stackVal.address(), 1)
    {}
    inline MutArrayHandle(MutHandle<T>& handle)
      : MutArrayHandle(handle.address(), 1)
    {}
    inline MutArrayHandle(LocalArray<T>& localArray)
      : MutArrayHandle(localArray.baseAddress(), localArray.length())
    {}

    inline static MutArrayHandle<T> FromTrackedLocation(T* valAddr,
                                                        uint32_t length)
    {
        return MutArrayHandle<T>(valAddr, length);
    }

    static MutArrayHandle<T> Empty() {
        return MutArrayHandle(nullptr, 0);
    }

    inline uint32_t length() const {
        return length_;
    }

    inline T* ptr() const {
        return valAddr_;
    }

    inline T& get(uint32_t i) const {
        WH_ASSERT(i < length_);
        return valAddr_[i];
    }

    inline void set(uint32_t i, T const& other) const {
        WH_ASSERT(i < length_);
        valAddr_[i] = other;
    }
    inline void set(uint32_t i, T&& other) const {
        WH_ASSERT(i < length_);
        valAddr_[i] = other;
    }

    inline Handle<T> operator [](uint32_t idx) const {
        return handle(idx);
    }
    inline MutHandle<T> operator [](uint32_t idx) {
        return mutHandle(idx);
    }

    Handle<T> handle(uint32_t idx) const;
    MutHandle<T> mutHandle(uint32_t idx);
};


//
// Handles are const pointers to values rooted on the stack with
// Local.
//
template <typename T>
class Handle
{
    template <typename U> friend class Handle;
    template <typename U> friend class ArrayHandle;
    template <typename U> friend class MutArrayHandle;

    typedef typename DerefTraits<T>::Type DerefType;
    typedef typename DerefTraits<T>::ConstType ConstDerefType;

  private:
    T const* const valAddr_;

    inline Handle(T const* valAddr)
      : valAddr_(valAddr)
    {}

  public:
    inline Handle(Handle<T> const& other)
      : valAddr_(other.valAddr_)
    {}
    inline Handle(Handle<T>&& other)
      : valAddr_(other.valAddr_)
    {}
    inline Handle(MutHandle<T> const& mutHandle)
      : valAddr_(mutHandle.address())
    {}
    inline Handle(Local<T> const& stackVal)
      : valAddr_(stackVal.address())
    {}
    inline Handle(LocalArray<T> const& stackArray, uint32_t idx)
      : valAddr_(stackArray->address(idx))
    {}
    inline Handle(StackField<T> const& stackField)
      : valAddr_(stackField.address())
    {}

    inline static Handle FromTrackedLocation(T const* valAddr)
    {
        return Handle(valAddr);
    }
    template <typename U>
    inline static Handle Convert(Handle<U> const& other)
    {
        static_assert(std::is_convertible<U, T>::value,
                      "U is not convertible to T.");
        return Handle<T>(reinterpret_cast<T const*>(other.address()));
    }

    template <typename U>
    inline Handle<U> convertTo()
    {
        static_assert(std::is_convertible<T, U>::value,
                      "T is not convertible to U.");
        return Handle<U>(reinterpret_cast<U const*>(address()));
    }

    template <typename U>
    inline Handle<U> upConvertTo()
    {
        static_assert(std::is_convertible<U, T>::value,
                      "U is not convertible to T.");
        return Handle<U>(reinterpret_cast<U const*>(address()));
    }

    inline T const& get() const {
        return *valAddr_;
    }

    inline T const* address() const {
        return valAddr_;
    }

    inline operator T const&() const {
        return get();
    }

    inline T const* operator &() const {
        return address();
    }

    inline ConstDerefType* operator ->() const {
        return DerefTraits<T>::Deref(*valAddr_);
    }

    Handle<T> const& operator =(Handle<T> const& other) = delete;
};

template <typename T>
class ArrayHandle
{
  private:
    T const* const valAddr_;
    uint32_t length_;

    inline ArrayHandle(T const* valAddr, uint32_t length)
      : valAddr_(valAddr),
        length_(length)
    {}

  public:
    inline ArrayHandle(Local<T>& stackVal)
      : ArrayHandle(stackVal.address(), 1)
    {}
    inline ArrayHandle(Handle<T>& handle)
      : ArrayHandle(handle.address(), 1)
    {}
    inline ArrayHandle(LocalArray<T>& localArray)
      : ArrayHandle(localArray.baseAddress(), localArray.length())
    {}

    inline ArrayHandle(MutArrayHandle<T> const& mutHandle)
      : ArrayHandle(mutHandle.valAddr_, mutHandle.length)
    {}

    inline static ArrayHandle<T> FromTrackedLocation(T const* valAddr,
                                                     uint32_t length)
    {
        return ArrayHandle<T>(valAddr, length);
    }

    static ArrayHandle<T> Empty() {
        return ArrayHandle(nullptr, 0);
    }

    inline T const* ptr() const {
        return valAddr_;
    }

    inline uint32_t length() const {
        return length_;
    }

    inline T const& get(uint32_t i) const {
        WH_ASSERT(i < length_);
        return valAddr_[i];
    }

    inline Handle<T> operator [](uint32_t idx) const {
        return handle(idx);
    }

    Handle<T> handle(uint32_t idx) const;
};

template <typename T>
inline Handle<T>
MutArrayHandle<T>::handle(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    return Handle<T>(&valAddr_[idx]);
}

template <typename T>
inline MutHandle<T>
MutArrayHandle<T>::mutHandle(uint32_t idx)
{
    WH_ASSERT(idx < length());
    return MutHandle<T>(&valAddr_[idx]);
}

template <typename T>
inline Handle<T>
ArrayHandle<T>::handle(uint32_t idx) const
{
    WH_ASSERT(idx < length());
    return Handle<T>(&valAddr_[idx]);
}


} // namespace Whisper

#endif // WHISPER__GC__HANDLE_HPP

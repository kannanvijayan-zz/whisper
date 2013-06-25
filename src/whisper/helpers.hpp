#ifndef WHISPER__HELPERS_HPP
#define WHISPER__HELPERS_HPP

#include "common.hpp"
#include "debug.hpp"
#include <new>

namespace Whisper {

// Check if an integer type is a power of two.
template <typename IntT>
inline bool IsPowerOfTwo(IntT value) {
    return (value & (value - 1)) == 0;
}

// Align integer types
template <typename IntT>
inline IntT AlignIntDown(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return value - (value & (align -1));
}

template <typename IntT>
inline IntT AlignIntUp(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return value + (align - (value & (align -1)));
}

// Align pointer types
template <typename PtrT, typename IntT>
inline PtrT *AlignPtrDown(PtrT *ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return WordToPtr<PtrT>(AlignIntDown<word_t>(PtrToWord(ptr), align));
}

template <typename PtrT, typename IntT>
inline PtrT *AlignPtrUp(PtrT *ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return WordToPtr<PtrT>(AlignIntUp<word_t>(PtrToWord(ptr), align));
}

// Max of two integers.
template <typename IntT>
inline constexpr IntT Max(IntT a, IntT b) {
    return (a >= b) ? a : b;
}

template <typename IntT>
inline constexpr IntT Min(IntT a, IntT b) {
    return (a <= b) ? a : b;
}

// Maybe class.
template <typename T>
class Maybe
{
  private:
    alignas(alignof(T)) char data_[sizeof(T)];
    bool hasValue_;

    inline T *ptr() {
        return reinterpret_cast<T *>(&data_[0]);
    }
    inline const T *ptr() const {
        return reinterpret_cast<const T *>(&data_[0]);
    }

  public:
    inline Maybe()
      : hasValue_(false)
    {}

    inline Maybe(const T &t)
      : hasValue_(true)
    {
        new (ptr()) T(t);
    }

    template <typename... ARGS>
    inline Maybe(ARGS... args)
      : hasValue_(true)
    {
        new (ptr()) T(args...);
    }

    inline bool hasValue() const {
        return hasValue_;
    }

    inline const T &value() const {
        WH_ASSERT(hasValue());
        return *(ptr());
    }
    inline T &value() {
        WH_ASSERT(hasValue());
        return *(ptr());
    }

    inline explicit operator bool() const {
        return hasValue();
    }

    inline operator const T *() const {
        return &value();
    }
    inline operator T *() {
        return &value();
    }

    inline const T *operator ->() const {
        return &value();
    }
    inline T *operator ->() {
        return &value();
    }

    inline const T &operator *() const {
        return value();
    }
    inline T &operator *() {
        return value();
    }

    inline const T &operator =(const T &val) {
        if (hasValue_)
            *ptr() = val;
        else
            new (ptr()) T(val);
        return val;
    }
};

// Either class.
template <typename T, typename U>
class Either
{
  private:
    static constexpr unsigned Size = Max(sizeof(T), sizeof(U));
    alignas(Max(alignof(T), alignof(U))) char data_[Size];
    bool hasFirst_;

    inline T *firstPtr() {
        return reinterpret_cast<T *>(&data_[0]);
    }
    inline const T *firstPtr() const {
        return reinterpret_cast<const T *>(&data_[0]);
    }

    inline T *secondPtr() {
        return reinterpret_cast<T *>(&data_[0]);
    }
    inline const T *secondPtr() const {
        return reinterpret_cast<const T *>(&data_[0]);
    }

  public:
    inline Either(const T &t)
      : hasFirst_(true)
    {
        new (firstPtr()) T(t);
    }

    inline Either(const U &u)
      : hasFirst_(false)
    {
        new (secondPtr()) U(u);
    }

    inline bool hasFirst() const {
        return hasFirst_;
    }
    inline bool hasSecond() const {
        return !hasFirst_;
    }

    inline const T &firstValue() const {
        WH_ASSERT(hasFirst());
        return *(firstPtr());
    }
    inline T &firstValue() {
        WH_ASSERT(hasFirst());
        return *(firstPtr());
    }

    inline const T &secondValue() const {
        WH_ASSERT(hasSecond());
        return *(secondPtr());
    }
    inline T &secondValue() {
        WH_ASSERT(hasSecond());
        return *(secondPtr());
    }

    inline const T &operator =(const T &val) {
        // Destroy existing second value of necessary.
        if (!hasFirst_) {
            secondPtr()->~U();
            hasFirst_ = true;
        }
            
        *firstPtr() = val;
        return val;
    }

    inline const U &operator =(const U &val) {
        // Destroy existing first value of necessary.
        if (hasFirst_) {
            firstPtr()->~T();
            hasFirst_ = false;
        }

        *secondPtr() = val;
    }
};

// OneOf class.
template <typename... TS>
class OneOf
{
  private:
    template <typename T>
    static constexpr size_t MaxSizeOf() {
        return sizeof(T);
    }
    template <typename T1, typename T2, typename... TX>
    static constexpr size_t MaxSizeOf() {
        return Max(sizeof(T1), MaxSizeOf<T2, TX...>());
    }


    template <typename T>
    static constexpr size_t MaxAlignOf() {
        return alignof(T);
    }
    template <typename T1, typename T2, typename... TX>
    static constexpr size_t MaxAlignOf() {
        return Max(alignof(T1), MaxAlignOf<T2, TX...>());
    }


    template <unsigned N, typename... TX> struct PickType {};

    template <typename T, typename... TX>
    struct PickType<0, T, TX...> {
        typedef T Type;
    };

    template <unsigned N, typename T, typename... TX>
    struct PickType<N, T, TX...> {
        typedef typename PickType<N-1, TX...>::Type Type;
    };

    template <unsigned N>
    using T = typename PickType<N, TS...>::Type;


    template <unsigned N>
    inline T<N> *nthPtr() {
        return reinterpret_cast<T<N> *>(&data_[0]);
    }
    template <unsigned N>
    inline const T<N> *nthPtr() const {
        return reinterpret_cast<const T<N> *>(&data_[0]);
    }


    alignas(MaxAlignOf<TS...>())
    char data_[MaxSizeOf<TS...>()];

    unsigned which_;

    template <unsigned N>
    struct Dummy {};

    template <unsigned N>
    inline OneOf(const T<N> &t, Dummy<N>)
      : which_(N)
    {
        new (nthPtr<N>()) T<N>(t);
    }
  public:
    template <unsigned N>
    static inline OneOf Make(const T<N> &t) {
        return OneOf(t, Dummy<N>());
    }

    inline bool isNth(unsigned n) const {
        return which_ == n;
    }

    template <unsigned N>
    inline const T<N> &nthValue() const {
        WH_ASSERT(isNth(N));
        return *(nthPtr<N>());
    }

    template <unsigned N>
    inline T<N> &nthValue() {
        WH_ASSERT(isNth(N));
        return *(nthPtr<N>());
    }

    template <unsigned N>
    inline const T<N> &setNth(const T<N> &val) {
        // If previous type was different, destruct it.
        if (!isNth(N)) {
            nthPtr<N>()->~T<N>();
            which_ = N;
        }

        *nthPtr<N>() = val;
    }
};


} // namespace Whisper

#endif // WHISPER__HELPERS_HPP

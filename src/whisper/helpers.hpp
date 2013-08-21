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
inline bool IsIntAligned(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return (value & (align - 1)) == 0;
}

template <typename IntT>
inline IntT AlignIntDown(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return value - (value & (align - 1));
}

template <typename IntT>
inline IntT AlignIntUp(IntT value, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    IntT vmod = value & (align - 1);
    return value + (vmod ? (align - vmod) : 0);
}

// Align pointer types
template <typename PtrT, typename IntT>
inline bool IsPtrAligned(PtrT *ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return IsIntAligned<word_t>(PtrToWord(ptr), align);
}

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

// Generation of integer types corresponding to a given size
template <unsigned Sz> struct IntTypeBySize {};
template <> struct IntTypeBySize<8> {
    typedef int8_t  Signed;
    typedef uint8_t  Unsigned;
};
template <> struct IntTypeBySize<16> {
    typedef int16_t  Signed;
    typedef uint16_t  Unsigned;
};
template <> struct IntTypeBySize<32> {
    typedef int32_t  Signed;
    typedef uint32_t  Unsigned;
};
template <> struct IntTypeBySize<64> {
    typedef int64_t  Signed;
    typedef uint64_t  Unsigned;
};

// Rotate integers.
template <typename IntT>
inline IntT RotateLeft(IntT val, unsigned rotate) {
    constexpr unsigned Size = sizeof(IntT);
    WH_ASSERT(rotate < (Size * 8));
    typedef typename IntTypeBySize<Size>::Unsigned UIntT;
    UIntT uval = static_cast<UIntT>(val);
    uval = (uval << rotate) | (uval >> ((Size*8) - rotate));
    return static_cast<IntT>(uval);
}

template <typename IntT>
inline IntT RotateRight(IntT val, unsigned rotate) {
    constexpr unsigned Size = sizeof(IntT);
    WH_ASSERT(rotate < (Size * 8));
    typedef typename IntTypeBySize<Size>::Unsigned UIntT;
    UIntT uval = static_cast<UIntT>(val);
    uval = (uval >> rotate) | (uval << ((Size*8) - rotate));
    return static_cast<IntT>(uval);
}

// Convert doubles to uint64_t representation and back.
inline uint64_t DoubleToInt(double d) {
    union {
        double dval;
        uint64_t ival;
    } u;
    u.dval = d;
    return u.ival;
}
inline double IntToDouble(uint64_t i) {
    union {
        uint64_t ival;
        double dval;
    } u;
    u.ival = i;
    return u.dval;
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

// Divide-up
template <typename IntT>
inline constexpr IntT DivUp(IntT a, IntT b) {
    return (a / b) + !!(a % b);
}

// Maybe class.
template <typename T>
class Maybe
{
  private:
    alignas(alignof(T)) char data_[sizeof(T)];
    bool hasValue_;

    T *ptr() {
        return reinterpret_cast<T *>(&data_[0]);
    }
    const T *ptr() const {
        return reinterpret_cast<const T *>(&data_[0]);
    }

  public:
    Maybe()
      : hasValue_(false)
    {}

    Maybe(const T &t)
      : hasValue_(true)
    {
        new (ptr()) T(t);
    }

    template <typename... ARGS>
    Maybe(ARGS... args)
      : hasValue_(true)
    {
        new (ptr()) T(args...);
    }

    bool hasValue() const {
        return hasValue_;
    }

    const T &value() const {
        WH_ASSERT(hasValue());
        return *(ptr());
    }
    T &value() {
        WH_ASSERT(hasValue());
        return *(ptr());
    }

    operator const T *() const {
        return hasValue_ ? &value() : nullptr;
    }
    operator T *() {
        return hasValue_ ? &value() : nullptr;
    }

    const T *operator ->() const {
        return &value();
    }
    T *operator ->() {
        return &value();
    }

    const T &operator *() const {
        return value();
    }
    T &operator *() {
        return value();
    }

    const T &operator =(const T &val) {
        if (hasValue_) {
            *ptr() = val;
        } else {
            new (ptr()) T(val);
            hasValue_ = true;
        }
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

    T *firstPtr() {
        return reinterpret_cast<T *>(&data_[0]);
    }
    const T *firstPtr() const {
        return reinterpret_cast<const T *>(&data_[0]);
    }

    T *secondPtr() {
        return reinterpret_cast<T *>(&data_[0]);
    }
    const T *secondPtr() const {
        return reinterpret_cast<const T *>(&data_[0]);
    }

  public:
    Either(const T &t)
      : hasFirst_(true)
    {
        new (firstPtr()) T(t);
    }

    Either(const U &u)
      : hasFirst_(false)
    {
        new (secondPtr()) U(u);
    }

    bool hasFirst() const {
        return hasFirst_;
    }
    bool hasSecond() const {
        return !hasFirst_;
    }

    const T &firstValue() const {
        WH_ASSERT(hasFirst());
        return *(firstPtr());
    }
    T &firstValue() {
        WH_ASSERT(hasFirst());
        return *(firstPtr());
    }

    const T &secondValue() const {
        WH_ASSERT(hasSecond());
        return *(secondPtr());
    }
    T &secondValue() {
        WH_ASSERT(hasSecond());
        return *(secondPtr());
    }

    const T &operator =(const T &val) {
        // Destroy existing second value of necessary.
        if (!hasFirst_) {
            secondPtr()->~U();
            hasFirst_ = true;
        }
            
        *firstPtr() = val;
        return val;
    }

    const U &operator =(const U &val) {
        // Destroy existing first value of necessary.
        if (hasFirst_) {
            firstPtr()->~T();
            hasFirst_ = false;
        }

        *secondPtr() = val;
    }
};


} // namespace Whisper

#endif // WHISPER__HELPERS_HPP

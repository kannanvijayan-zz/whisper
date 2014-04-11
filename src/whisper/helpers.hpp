#ifndef WHISPER__HELPERS_HPP
#define WHISPER__HELPERS_HPP

#include "common.hpp"
#include "debug.hpp"
#include <new>
#include <limits>
#include <type_traits>

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
template <unsigned Sz> struct IntTypeByBits {};
template <> struct IntTypeByBits<8> {
    typedef int8_t  Signed;
    typedef uint8_t  Unsigned;
};
template <> struct IntTypeByBits<16> {
    typedef int16_t  Signed;
    typedef uint16_t  Unsigned;
};
template <> struct IntTypeByBits<32> {
    typedef int32_t  Signed;
    typedef uint32_t  Unsigned;
};
template <> struct IntTypeByBits<64> {
    typedef int64_t  Signed;
    typedef uint64_t  Unsigned;
};

// Rotate integers.
template <typename IntT>
inline IntT RotateLeft(IntT val, unsigned rotate) {
    constexpr unsigned Bits = sizeof(IntT) * 8;
    WH_ASSERT(rotate < Bits);
    typedef typename IntTypeByBits<Bits>::Unsigned UIntT;
    UIntT uval = static_cast<UIntT>(val);
    uval = (uval << rotate) | (uval >> (Bits - rotate));
    return static_cast<IntT>(uval);
}

template <typename IntT>
inline IntT RotateRight(IntT val, unsigned rotate) {
    constexpr unsigned Bits = sizeof(IntT) * 8;
    WH_ASSERT(rotate < Bits);
    typedef typename IntTypeByBits<Bits>::Unsigned UIntT;
    UIntT uval = static_cast<UIntT>(val);
    uval = (uval >> rotate) | (uval << (Bits - rotate));
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

inline unsigned GetDoubleExponentField(double d) {
    return (DoubleToInt(d) >> 52) & 0x7FFu;
}

inline uint64_t GetDoubleMantissaField(double d) {
    return DoubleToInt(d) & ((ToUInt64(1) << 52) - 1);
}

inline bool GetDoubleSign(double d) {
    return DoubleToInt(d) >> 63;
}

inline bool DoubleIsNaN(double d) {
    return d != d;
}

inline bool DoubleIsPosInf(double d) {
    return d == std::numeric_limits<double>::infinity();
}

inline bool DoubleIsNegInf(double d) {
    return d == -std::numeric_limits<double>::infinity();
}

inline bool DoubleIsNegZero(double d) {
    return d == 0.0 && GetDoubleSign(d);
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

template <typename T>
constexpr unsigned IntBits(bool includeSign=true) {
    static_assert(std::numeric_limits<T>::is_specialized,
                  "Type is not a number.");
    static_assert(std::numeric_limits<T>::is_integer,
                  "Type is not an integer.");
    return std::numeric_limits<T>::digits +
           (includeSign && std::numeric_limits<T>::is_signed ? 1 : 0);
}

// Helper class for manipulating bitfields.
template <typename WORD, typename FIELD, unsigned BITS, unsigned SHIFT>
class BaseBitfield
{
    // Ensure that WORD is an unsigned integer.
    static_assert(std::numeric_limits<WORD>::is_specialized,
                  "Word type is not a number.");
    static_assert(std::numeric_limits<WORD>::is_integer,
                  "Word type is not an integer.");
    static_assert(!std::numeric_limits<WORD>::is_signed,
                  "Word type is not unsigned.");

    // Ensure that FIELD is an integer.
    static_assert(std::numeric_limits<FIELD>::is_specialized,
                  "Field type is not a number.");
    static_assert(std::numeric_limits<FIELD>::is_integer,
                  "Field type is not an integer.");

    // Ensure that FIELD can hold BITS bits.
    static_assert(IntBits<FIELD>() >= BITS,
                  "Field is not big enough to hold bits.");

    // Ensure that WORD can hold BITS bits at SHIFT position.
    static_assert(IntBits<WORD>() >= SHIFT + BITS,
                  "Word is not big enough to hold field.");

  public:
    typedef WORD WordT;
    typedef FIELD FieldT;
    static constexpr unsigned Bits = BITS;
    static constexpr unsigned Shift = SHIFT;

    static constexpr bool SignedField = std::numeric_limits<FieldT>::is_signed;

    static constexpr WordT LowMask = (static_cast<WordT>(1) << Bits) - 1;
    static constexpr WordT HighMask = LowMask << Shift;

    static constexpr WordT SignBit = static_cast<WordT>(1) << (Bits - 1);

    // Minimum value for signed field: 11..11100..00
    //                                       -------
    //                                         BITS
    static constexpr FieldT MinValue =
        SignedField ? static_cast<FieldT>(-1) << (Bits - 1) : 0;

    // Maximum value for signed field: 00..00011..11
    //                                       -------
    //                                         BITS
    static constexpr FieldT MaxValue = 
        SignedField ? (static_cast<FieldT>(1) << (Bits - 1)) - 1
                    : static_cast<FieldT>(LowMask);
                   

  protected:
    WordT &word_;

  public:
    inline BaseBitfield(WordT &word) : word_(word) {}

    // Get the value from the bitfield.
    inline FieldT value() const {
        typedef typename std::remove_const<WordT>::type RawWordT;
        RawWordT result = (word_ >> Shift) & LowMask;
        // Sign extend if necessary
        if (SignedField && (result & SignBit))
            result |= ~LowMask;
        return static_cast<FieldT>(result);
    }

    inline operator FieldT() const {
        return value();
    }

    // Check if an integer value fits into the field.
    inline static bool ValueFits(FieldT value) {
        return (value >= MinValue) && (value <= MaxValue);
    }
};

template <typename WORD, typename FIELD, unsigned BITS, unsigned SHIFT>
class ConstBitfield : public BaseBitfield<const WORD, FIELD, BITS, SHIFT>
{
    typedef BaseBitfield<const WORD, FIELD, BITS, SHIFT> BaseType;

  public:
    inline ConstBitfield(const WORD &word) : BaseType(word) {}
};

template <typename WORD, typename FIELD, unsigned BITS, unsigned SHIFT>
class Bitfield : public BaseBitfield<WORD, FIELD, BITS, SHIFT>
{
    typedef BaseBitfield<WORD, FIELD, BITS, SHIFT> BaseType;

  public:
    typedef ConstBitfield<const WORD, FIELD, BITS, SHIFT> Const;

    inline Bitfield(WORD &word) : BaseType(word) {}

    inline void initValue(FIELD val) {
        WH_ASSERT(BaseType::ValueFits(val));
        WORD fieldVal = static_cast<FIELD>(val) & BaseType::LowMask;
        // initValue can assume that field bits are already zero,
        // so bits do not need to be zeroed before or-mask is applied.
        this->word_ |= fieldVal << BaseType::Shift;
    }

    inline void setValue(FIELD val) {
        WH_ASSERT(BaseType::ValueFits(val));

        WORD fieldVal = static_cast<FIELD>(val) & BaseType::LowMask;
        this->word_ &= ~BaseType::HighMask;
        this->word_ |= fieldVal << BaseType::Shift;
    }

    inline FIELD operator =(FIELD val) {
        setValue(val);
        return val;
    }
};


} // namespace Whisper

#endif // WHISPER__HELPERS_HPP

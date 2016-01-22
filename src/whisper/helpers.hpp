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
inline bool IsPtrAligned(PtrT* ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return IsIntAligned<word_t>(PtrToWord(ptr), align);
}

template <typename PtrT, typename IntT>
inline PtrT* AlignPtrDown(PtrT* ptr, IntT align) {
    WH_ASSERT(IsPowerOfTwo(align));
    return WordToPtr<PtrT>(AlignIntDown<word_t>(PtrToWord(ptr), align));
}

template <typename PtrT, typename IntT>
inline PtrT* AlignPtrUp(PtrT* ptr, IntT align) {
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

template <typename IntT, IntT Val1>
inline constexpr IntT ConstExprMax() {
    return Val1;
}
template <typename IntT, IntT Val1, IntT Val2, IntT... Vals>
inline constexpr IntT ConstExprMax() {
    return (Val1 >= Val2) ? ConstExprMax<IntT, Val1, Vals...>()
                          : ConstExprMax<IntT, Val2, Vals...>();
}

template <typename IntT>
inline constexpr IntT Min(IntT a, IntT b) {
    return (a <= b) ? a : b;
}

template <typename IntT, IntT Val1>
inline constexpr IntT ConstExprMin() {
    return Val1;
}
template <typename IntT, IntT Val1, IntT Val2, IntT... Vals>
inline constexpr IntT ConstExprMin() {
    return (Val1 <= Val2) ? ConstExprMin<IntT, Val1, Vals...>()
                          : ConstExprMin<IntT, Val2, Vals...>();
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

    T* ptr() {
        return reinterpret_cast<T*>(&data_[0]);
    }
    T const* ptr() const {
        return reinterpret_cast<T const*>(&data_[0]);
    }

    Maybe()
      : hasValue_(false)
    {}

    Maybe(T const& t)
      : hasValue_(true)
    {
        new (ptr()) T(t);
    }

    Maybe(T&& t)
      : hasValue_(true)
    {
        new (ptr()) T(std::move(t));
    }
  public:
    Maybe(Maybe<T> const& t)
      : hasValue_(t.hasValue())
    {
        if (t.hasValue())
            new (ptr()) T(t.value());
    }
    Maybe(Maybe<T>&& t)
      : hasValue_(t.hasValue())
    {
        if (t.hasValue())
            new (ptr()) T(std::move(t.value()));
    }

    ~Maybe() {
        if (hasValue_)
            ptr()->~T();
    }

    static Maybe<T> None() {
        return Maybe<T>();
    }
    static Maybe<T> Some(T const& t) {
        return Maybe<T>(t);
    }
    static Maybe<T> Some(T&& t) {
        return Maybe<T>(std::move(t));
    }

    bool hasValue() const {
        return hasValue_;
    }

    T const& value() const {
        WH_ASSERT(hasValue());
        return *(ptr());
    }
    T& value() {
        WH_ASSERT(hasValue());
        return *(ptr());
    }

    operator T const* () const {
        WH_ASSERT(hasValue());
        return &value();
    }
    operator T* () {
        WH_ASSERT(hasValue());
        return &value();
    }

    T const* operator ->() const {
        return &value();
    }
    T* operator ->() {
        return &value();
    }

    T const& operator *() const {
        return value();
    }
    T& operator *() {
        return value();
    }

    T getWithFallback(T fallback) const {
        return hasValue() ? value() : fallback;
    }

    Maybe<T>& operator =(T const& val) {
        if (hasValue_) {
            *ptr() = val;
        } else {
            new (ptr()) T(val);
            hasValue_ = true;
        }
        return *this;
    }
    Maybe<T>& operator =(T&& val) {
        if (hasValue_) {
            *ptr() = std::move(val);
        } else {
            new (ptr()) T(std::move(val));
            hasValue_ = true;
        }
        return *this;
    }
    void clear() {
        if (hasValue_) {
            ptr()->~T();
            hasValue_ = false;
        }
    }

    Maybe<T>& operator =(Maybe<T> const& val) {
        if (val.hasValue())
            *this = val.value();
        else
            clear();
        return *this;
    }

    Maybe<T>& operator =(Maybe<T>&& val) {
        if (val.hasValue())
            *this = std::move(val.value());
        else
            clear();
        return *this;
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
template <typename WordT_, typename FieldT_, unsigned Bits_, unsigned Shift_>
class BaseBitfield
{
    // Ensure that WordT_ is an unsigned integer.
    static_assert(std::numeric_limits<WordT_>::is_specialized,
                  "Word type is not a number.");
    static_assert(std::numeric_limits<WordT_>::is_integer,
                  "Word type is not an integer.");
    static_assert(!std::numeric_limits<WordT_>::is_signed,
                  "Word type is not unsigned.");

    // Ensure that FieldT_ is an integer.
    static_assert(std::numeric_limits<FieldT_>::is_specialized,
                  "Field type is not a number.");
    static_assert(std::numeric_limits<FieldT_>::is_integer,
                  "Field type is not an integer.");

    // Ensure that FieldT_ can hold Bits_ bits.
    static_assert(IntBits<FieldT_>() >= Bits_,
                  "Field is not big enough to hold bits.");

    // Ensure that WordT_ can hold Bits_ bits at Shift_ position.
    static_assert(IntBits<WordT_>() >= Shift_ + Bits_,
                  "Word is not big enough to hold field.");

  public:
    typedef WordT_ WordT;
    typedef FieldT_ FieldT;
    static constexpr unsigned Bits = Bits_;
    static constexpr unsigned Shift = Shift_;

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
    WordT& word_;

  public:
    inline BaseBitfield(WordT& word) : word_(word) {}

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

    inline static WordT Lift(FieldT value) {
        WH_ASSERT(ValueFits(value));
        return (value & LowMask) << Shift;
    }
};

template <typename WordT, typename FieldT, unsigned Bits, unsigned Shift>
class ConstBitfield : public BaseBitfield<const WordT, FieldT, Bits, Shift>
{
    typedef BaseBitfield<const WordT, FieldT, Bits, Shift> BaseType;

  public:
    inline ConstBitfield(WordT const& word) : BaseType(word) {}
};

template <typename WordT, typename FieldT, unsigned Bits, unsigned Shift>
class Bitfield : public BaseBitfield<WordT, FieldT, Bits, Shift>
{
    typedef BaseBitfield<WordT, FieldT, Bits, Shift> BaseType;

  public:
    typedef ConstBitfield<const WordT, FieldT, Bits, Shift> Const;

    inline Bitfield(WordT& word) : BaseType(word) {}

    inline void initValue(FieldT val) {
        WH_ASSERT(BaseType::ValueFits(val));
        WordT fieldVal = static_cast<FieldT>(val) & BaseType::LowMask;
        // initValue can assume that field bits are already zero,
        // so bits do not need to be zeroed before or-mask is applied.
        this->word_ |= fieldVal << BaseType::Shift;
    }

    inline void setValue(FieldT val) {
        WH_ASSERT(BaseType::ValueFits(val));

        WordT fieldVal = static_cast<FieldT>(val) & BaseType::LowMask;
        this->word_ &= ~BaseType::HighMask;
        this->word_ |= fieldVal << BaseType::Shift;
    }

    inline FieldT operator =(FieldT val) {
        setValue(val);
        return val;
    }
};


} // namespace Whisper

#endif // WHISPER__HELPERS_HPP

#ifndef WHISPER__VM__ARRAY_HPP
#define WHISPER__VM__ARRAY_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"

#include <new>

namespace Whisper {

namespace VM {
    template <typename T> class Array;
    template <bool ONSTACK, typename T, uint32_t N> class SizedArray;
};

// Specialize Array for SlabThingTraits
template <typename T>
struct SlabThingTraits<VM::Array<T>>
{
    SlabThingTraits() = delete;

    static constexpr bool SPECIALIZED = true;

    // Support any constructor calls with an initial uint32_t argument,
    // which is assumed to be the number of elements in the array.
    template <typename... Args>
    static uint32_t SIZE_OF(uint32_t size, Args... args) {
        return sizeof(VM::Array<T>) + (size * sizeof(T));
    }
};

// Specialize SizedArray for SlabThingTraits
template <bool ONSTACK, typename T, uint32_t N>
struct SlabThingTraits<VM::SizedArray<ONSTACK, T, N>>
{
    SlabThingTraits() = delete;

    static constexpr bool SPECIALIZED = true;

    // Support any constructor calls with an initial uint32_t argument,
    // which is assumed to be the number of elements in the array.
    template <typename... Args>
    static uint32_t SIZE_OF(Args... args) {
        return sizeof(VM::SizedArray<ONSTACK, T, N>);
    }
};

// Specialize AllocTraits for all primitive-type stack and heap arrays to have
// UntracedThing format.

// Provide pre-specializations of AllocFormatTraits for common arrays.
#define PRIM_ARRAY_ALLOC_TRAITS_DEF_(type) \
    template <> \
    struct AllocTraits<VM::Array<type>> { \
        AllocTraits() = delete; \
        static constexpr bool SPECIALIZED = true; \
        static constexpr AllocFormat FORMAT = AllocFormat::UntracedThing; \
        typedef VM::Array<type> T_; \
        typedef VM::Array<type> DEREF_TYPE; \
        static DEREF_TYPE *DEREF(T_ &t) { return &t; } \
        static const DEREF_TYPE *DEREF(const T_ &t) { \
            return &t; \
        } \
    }; \
    template <bool ONSTACK, uint32_t N> \
    struct AllocTraits<VM::SizedArray<ONSTACK, type, N>> { \
        AllocTraits() = delete; \
        static constexpr bool SPECIALIZED = true; \
        static constexpr AllocFormat FORMAT = AllocFormat::UntracedThing; \
        typedef VM::SizedArray<ONSTACK, type, N> T_; \
        typedef VM::SizedArray<ONSTACK, type, N> DEREF_TYPE; \
        static DEREF_TYPE *DEREF(T_ &t) { return &t; } \
        static const DEREF_TYPE *DEREF(const T_ &t) { \
            return &t; \
        } \
    };
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(bool);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(int8_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(uint8_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(int16_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(uint16_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(int32_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(uint32_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(int64_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(uint64_t);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(float);
    PRIM_ARRAY_ALLOC_TRAITS_DEF_(double);
#undef PRIM_ARRAY_ALLOC_TRAITS_DEF_


namespace VM {


//
// A slab-allocated fixed-length array
//
template <typename T>
class Array
{
  private:
    T vals_[0];

  public:
    inline Array(size_t len, const T *vals) {
        for (uint32_t i = 0; i < len; i++)
            new (&vals_[i]) T(vals[i]);
    }

    template <typename... Args>
    inline Array(size_t len, Args... args) {
        for (uint32_t i = 0; i < len; i++)
            new (&vals_[i]) T(std::forward<Args>(args)...);
    }

    inline uint32_t length() const {
        WH_ASSERT(AllocThing::From(this)->size() % sizeof(T) == 0);
        return AllocThing::From(this)->size() / sizeof(T);
    }

    inline const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }
    inline T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    inline T get(uint32_t idx) const {
        return vals_[idx];
    }

    inline void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());

        // If the array is not traced, setting is simple.
        if (SlabThingTraits<Array<T>>::TRACED)
            get(idx).set(val, this);
        else
            getRaw(idx) = val;
    }
};


//
// A slab-allocated fixed-length array with static size.
//
template <bool ONSTACK, typename T, uint32_t N>
class SizedArray
{
  private:
    T vals_[0];

    static constexpr FieldType FT = ONSTACK ? FieldType::Stack
                                            : FieldType::Heap;
    typedef typename ChooseField<FT, T>::TYPE FieldT;

  public:
    inline SizedArray(const T *vals) {
        for (uint32_t i = 0; i < N; i++)
            new (&vals_[i]) T(vals[i]);
    }

    template <typename... Args>
    inline SizedArray(Args... args) {
        for (uint32_t i = 0; i < N; i++)
            new (&vals_[i]) T(std::forward<Args>(args)...);
    }

    inline uint32_t length() const {
        return N;
    }

    inline const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }
    inline T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    inline T get(uint32_t idx) const {
        return vals_[idx];
    }

    template <uint32_t IDX>
    inline T get() const {
        static_assert(IDX < N, "Invalid index.");
        return vals_[IDX];
    }

    inline void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());
        reinterpret_cast<FieldT *>(&vals_[idx])->set(val, this);
    }

    template <uint32_t IDX>
    inline void set(uint32_t idx, const T &val) {
        static_assert(IDX < N, "Invalid index.");
        reinterpret_cast<FieldT *>(&vals_[IDX])->set(val, this);
    }
};

template <typename T, uint32_t N>
using SizedStackArray = SizedArray<true, T, N>;

template <typename T, uint32_t N>
using SizedHeapArray = SizedArray<false, T, N>;


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

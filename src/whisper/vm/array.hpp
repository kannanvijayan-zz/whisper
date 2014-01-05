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

    // ArrayTraits for a particular type T provides information about
    // T.
    template <typename T>
    struct ArrayTraits {
        ArrayTraits() = delete;
        static constexpr bool TRACED = true;
    };
#define UNTRACED_SPEC_(T) \
    template <> struct ArrayTraits<T> {  \
        ArrayTraits() = delete; \
        static constexpr bool TRACED = false; \
    }
    UNTRACED_SPEC_(bool);
    UNTRACED_SPEC_(int8_t);
    UNTRACED_SPEC_(uint8_t);
    UNTRACED_SPEC_(int16_t);
    UNTRACED_SPEC_(uint16_t);
    UNTRACED_SPEC_(int32_t);
    UNTRACED_SPEC_(uint32_t);
    UNTRACED_SPEC_(int64_t);
    UNTRACED_SPEC_(uint64_t);
    UNTRACED_SPEC_(float);
    UNTRACED_SPEC_(double);
#undef UNTRACED_SPEC_
};

// Specialize Array for SlabThingTraits
template <typename T>
struct SlabThingTraits<VM::Array<T>>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

// Specialize Array for AllocationTraits, using ArrayTraits
// Arrays are traced by default, but array specializations for primitive types
// are not traced.
template <typename T>
struct AllocationTraits<VM::Array<T>>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::Array;
    static constexpr bool TRACED = VM::ArrayTraits<T>::TRACED;
};


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
    static uint32_t CalculateSize(uint32_t length) {
        return sizeof(Array<T>) + (length * sizeof(T));
    }

    inline Array(const T *vals) {
        uint32_t len = length();
        for (uint32_t i = 0; i < len; i++)
            new (&vals_[i]) T(vals[i]);
    }

    template <typename... Args>
    inline Array(Args... args) {
        uint32_t len = length();
        for (uint32_t i = 0; i < len; i++)
            new (&vals_[i]) T(std::forward<Args>(args)...);
    }

    static Array<T> *Create(AllocationContext &cx, uint32_t length,
                            const T *vals)
    {
        return cx.createSized<Array<T>>(CalculateSize(length), vals);
    }

    template <typename... Args>
    static Array<T> *Create(AllocationContext &cx, uint32_t length,
                            Args... args)
    {
        return cx.createSized<Array<T>>(CalculateSize(length),
                                        std::forward<Args>(args)...);
    }

    inline uint32_t length() const {
        return SlabThing::From(this)->allocSize() / sizeof(T);
    }

    inline const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }
    inline T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    inline const Heap<T> &get(uint32_t idx) const {
        return reinterpret_cast<const Heap<T> &>(getRaw(idx));
    }
    inline Heap<T> &get(uint32_t idx) {
        return reinterpret_cast<Heap<T> &>(getRaw(idx));
    }

    inline void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());

        // If the array is not traced, setting is simple.
        if (AllocationTraits<Array<T>>::TRACED)
            get(idx).set(val, this);
        else
            getRaw(idx) = val;
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

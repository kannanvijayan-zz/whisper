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
};

// Specialize Array for SlabThingTraits
template <typename T>
struct SlabThingTraits<VM::Array<T>>
  : public SlabThingTraitsHelper<VM::Array<T>>
{};

// Specialize Array for AllocationTraits
// Arrays are traced by default, but array specializations for primitive types
// are not traced.
template <typename T>
struct AllocationTraits<VM::Array<T>>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::Array;
    static constexpr bool TRACED = true;
};
#define UNTRACED_SPEC_(T) \
    template <> struct AllocationTraits<VM::Array<T>> {  \
        static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::Array; \
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
        uint32_t sz = size();
        for (uint32_t i = 0; i < sz; i++)
            new (&vals_[i]) T(vals[i]);
    }

    template <typename... Args>
    inline Array(Args... args) {
        uint32_t sz = size();
        for (uint32_t i = 0; i < sz; i++)
            new (&vals_[i]) T(std::forward<Args>(args)...);
    }

    inline uint32_t size() const {
        return SlabThing::From(this)->allocSize() / sizeof(T);
    }

    inline const T &operator[](uint32_t idx) const {
        WH_ASSERT(idx < size());
        return vals_[idx];
    }

    inline void set(uint32_t idx, const T &val) {
        // If the array is not traced, setting is simple.
        if (!AllocationTraits<Array<T>>::TRACED) {
            vals_[idx] = val;
            return;
        }

        // Otherwise, cast the value to a Heap reference, and call
        // set() on it.
        reinterpret_cast<Heap<T> &>(vals_[idx]).set(val, this);
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

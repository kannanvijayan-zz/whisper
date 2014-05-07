#ifndef WHISPER__VM__ARRAY_HPP
#define WHISPER__VM__ARRAY_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "gc.hpp"

#include <new>

namespace Whisper {
namespace VM {

//
// A slab-allocated fixed-length array
//
template <typename T>
class Array
{
    // T must be a field type to be usable.
    static_assert(GC::FieldTraits<T>::Specialized,
                  "Underlying type is not field-specialized.");

  private:
    HeapField<T> vals_[0];

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

    inline uint32_t length() const;

    inline const T &getRaw(uint32_t idx) const;
    inline T &getRaw(uint32_t idx);

    inline T get(uint32_t idx) const;

    inline void set(uint32_t idx, const T &val);
};


} // namespace VM
} // namespace Whisper


//
// GC-Specializations for Array
//
namespace Whisper {
namespace GC {


template <typename T>
class HeapTraits<Array<T>>
{
    // The generic specialization of Array<T> demands that T itself
    // have either a stack of heap specialization.
    static_assert(Stack

    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = HeapTraits<T>::IsLeaf;
    static constexpr bool AllocFormat =
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

#ifndef WHISPER__VM__ARRAY_HPP
#define WHISPER__VM__ARRAY_HPP


#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
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
    friend class GC::TraceTraits<Array<T>>;

    // T must be a field type to be usable.
    static_assert(GC::FieldTraits<T>::Specialized,
                  "Underlying type is not field-specialized.");

  private:
    HeapField<T> vals_[0];

  public:
    Array(uint32_t len, const T *vals) {
        for (uint32_t i = 0; i < len; i++)
            new (&vals_[i]) T(vals[i]);
    }

    Array(uint32_t len, const T &val) {
        for (uint32_t i = 0; i < len; i++) {
            SpewMemoryWarn("Array construct %p %d/%d = %d",
                           this, i, len, int(val));
            vals_[i].init(this, val);
            // new (&vals_[i]) T(val);
        }
        SpewMemoryWarn("Array length %d", (int)length());
    }

    inline uint32_t length() const;

    const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    T get(uint32_t idx) const {
        return vals_[idx];
    }

    inline void set(uint32_t idx, const T &val);
    inline void set(uint32_t idx, T &&val);
};


} // namespace VM
} // namespace Whisper

// Include array template specializations for describing arrays to
// the GC.
#include "vm/array_specializations.hpp"


namespace Whisper {
namespace VM {


template <typename T>
inline uint32_t
Array<T>::length() const
{
    uint32_t size = GC::AllocThing::From(this)->size();
    WH_ASSERT(size % sizeof(T) == 0);
    return size / sizeof(T);
}

template <typename T>
inline void
Array<T>::set(uint32_t idx, const T &val)
{
    WH_ASSERT(idx < length());
    vals_[idx].set(val, this);
}

template <typename T>
inline void
Array<T>::set(uint32_t idx, T &&val)
{
    WH_ASSERT(idx < length());
    vals_[idx].set(val, this);
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

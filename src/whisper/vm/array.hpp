#ifndef WHISPER__VM__ARRAY_HPP
#define WHISPER__VM__ARRAY_HPP


#include "vm/core.hpp"

#include <new>

namespace Whisper {
namespace VM {


// Template to annotate types which are used as parameters to
// Array<>, so that we can derive an AllocFormat for the array of
// a particular type.
template <typename T>
struct ArrayTraits
{
    ArrayTraits() = delete;

    // Set to true for all specializations.
    static const bool Specialized = false;

    // Give an AllocFormat for an array of type T.
    //
    // static const GC::AllocFormat ArrayFormat;

    // Give a map-back type T' to map the array format back to
    // VM::Array<T'>
    // typedef SomeType T;
};


//
// A slab-allocated fixed-length array
//
template <typename T>
class Array
{
    friend struct GC::TraceTraits<Array<T>>;

    // T must be a field type to be usable.
    static_assert(GC::FieldTraits<T>::Specialized,
                  "Underlying type is not field-specialized.");
    static_assert(ArrayTraits<T>::Specialized,
                  "Underlying type does not have an array specialization.");

  private:
    HeapField<T> vals_[0];

  public:
    static uint32_t CalculateSize(uint32_t len) {
        return sizeof(Array<T>) + (sizeof(HeapField<T>) * len);
    }

    Array(uint32_t len, const T *vals) {
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(this, vals[i]);
    }

    Array(uint32_t len, const T &val) {
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(this, val);
    }

    Array(const Array<T> &other) {
        for (uint32_t i = 0; i < other.length(); i++)
            vals_[i].init(this, other.vals_[i]);
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

    void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());
        vals_[idx].set(val, this);
    }
    void set(uint32_t idx, T &&val) {
        WH_ASSERT(idx < length());
        vals_[idx].set(val, this);
    }
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

/*
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
*/


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

#ifndef WHISPER__VM__ARRAY_HPP
#define WHISPER__VM__ARRAY_HPP


#include "vm/core.hpp"

namespace Whisper {
namespace VM {


// Template to annotate types which are used as parameters to
// Array<>, so that we can derive an HeapFormat for the array of
// a particular type.
template <typename T>
struct ArrayTraits
{
    ArrayTraits() = delete;

    // Set to true for all specializations.
    static const bool Specialized = false;

    // Give an HeapFormat for an array of type T.
    //
    // static const HeapFormat ArrayFormat;

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
    friend struct TraceTraits<Array<T>>;

    // T must be a field type to be usable.
    static_assert(FieldTraits<T>::Specialized,
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
        // T should NOT be heap-allocated.
        static_assert(!HeapTraits<T>::Specialized,
                      "Heap-allocated objects must use ArrayHandle-based "
                      "constructor");
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(vals[i], this);
    }

    Array(uint32_t len, const T &val) {
        static_assert(!HeapTraits<T>::Specialized,
                      "Heap-allocated objects must use Handle-based "
                      "constructor");
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(val, this);
    }

    Array(ArrayHandle<T> vals) {
        WH_ASSERT(length() == vals.length());
        for (uint32_t i = 0; i < vals.length(); i++)
            vals_[i].init(vals[i], this);
    }

    Array(uint32_t len, Handle<T> val) {
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(val, this);
    }

    Array(Handle<Array<T> *> arr) {
        WH_ASSERT(length() == arr->length());
        for (uint32_t i = 0; i < arr->length(); i++)
            vals_[i].init(arr->vals_[i], this);
    }

    static inline Result<Array<T> *> Create(AllocationContext acx,
                                             uint32_t length,
                                             const T *vals);
    static inline Result<Array<T> *> Create(AllocationContext acx,
                                            uint32_t length,
                                            const T &val);
    static inline Result<Array<T> *> Create(AllocationContext acx,
                                            ArrayHandle<T> arr);
    static inline Result<Array<T> *> Create(AllocationContext acx,
                                            uint32_t length,
                                            Handle<T> val);
    static inline Result<Array<T> *> Create(AllocationContext acx,
                                            Handle<Array<T> *> other);

    inline uint32_t length() const;

    const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx].getRaw();
    }
    T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx].getRaw();
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
    uint32_t size = HeapThing::From(this)->size();
    WH_ASSERT(size % sizeof(T) == 0);
    return size / sizeof(T);
}

template <typename T>
/* static */ inline Result<Array<T> *>
Array<T>::Create(AllocationContext acx, uint32_t len, const T *vals)
{
    return acx.createSized<Array<T>>(CalculateSize(len), len, vals);
}

template <typename T>
/* static */ inline Result<Array<T> *>
Array<T>::Create(AllocationContext acx, uint32_t len, const T &val)
{
    return acx.createSized<Array<T>>(CalculateSize(len), len, val);
}

template <typename T>
/* static */ inline Result<Array<T> *>
Array<T>::Create(AllocationContext acx, ArrayHandle<T> arr)
{
    uint32_t len = arr.length();
    return acx.createSized<Array<T>>(CalculateSize(len), arr);
}

template <typename T>
/* static */ inline Result<Array<T> *>
Array<T>::Create(AllocationContext acx, uint32_t length, Handle<T> val)
{
    return acx.createSized<Array<T>>(CalculateSize(length), length, val);
}

template <typename T>
/* static */ inline Result<Array<T> *>
Array<T>::Create(AllocationContext acx, Handle<Array<T> *> other)
{
    uint32_t len = other->length();
    return acx.createSized<Array<T>>(CalculateSize(len), other);
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

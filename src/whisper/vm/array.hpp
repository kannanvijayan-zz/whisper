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

    uint32_t length() const {
        WH_ASSERT(GC::AllocThing::From(this)->size() % sizeof(T) == 0);
        return GC::AllocThing::From(this)->size() / sizeof(T);
    }

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
        vals_[idx].set(val, GC::AllocThing::From(this));
    }
};


// Template to annotate types which are used as parameters to
// Array<>.
template <typename T>
struct ArrayTraits
{
    ArrayTraits() = delete;

    // Set to true for all specializations.
    static const bool Specialized = false;

    // Give an AllocFormat for an array of type T.
    //
    // static const GC::AllocFormat ArrayFormat;
};

// Specialize arrays for primitive types.
#define DEF_ARRAY_TRAITS_(type, fmtName) \
    template <> \
    struct ArrayTraits<type> { \
        ArrayTraits() = delete; \
        static const bool Specialized = true; \
        static const GC::AllocFormat ArrayFormat = GC::AllocFormat::fmtName; \
    };

DEF_ARRAY_TRAITS_(uint8_t, UntracedThing);
DEF_ARRAY_TRAITS_(uint16_t, UntracedThing);
DEF_ARRAY_TRAITS_(uint32_t, UntracedThing);
DEF_ARRAY_TRAITS_(uint64_t, UntracedThing);
DEF_ARRAY_TRAITS_(int8_t, UntracedThing);
DEF_ARRAY_TRAITS_(int16_t, UntracedThing);
DEF_ARRAY_TRAITS_(int32_t, UntracedThing);
DEF_ARRAY_TRAITS_(int64_t, UntracedThing);
DEF_ARRAY_TRAITS_(float, UntracedThing);
DEF_ARRAY_TRAITS_(double, UntracedThing);

DEF_ARRAY_TRAITS_(GC::AllocThing *, AllocThingPointerArray);

#undef DEF_ARRAY_TRAITS_

// Specialize arrays for pointer types.
template <typename P>
struct ArrayTraits<P *> {
    static_assert(GC::HeapTraits<P>::Specialized,
                  "Underlying type of pointer is not a heap thing.");
    ArrayTraits() = delete;
    static const bool Specialized = true;
    static const GC::AllocFormat ArrayFormat =
        GC::AllocFormat::AllocThingPointerArray;
};


} // namespace VM
} // namespace Whisper


//
// GC-Specializations for Array
//
namespace Whisper {
namespace GC {


template <typename T>
struct HeapTraits<VM::Array<T>>
{
    // The generic specialization of Array<T> demands that T itself
    // have either a field specialization.
    static_assert(FieldTraits<T>::Specialized,
                  "Underlying type does not have a field specialization.");
    static_assert(VM::ArrayTraits<T>::Specialized,
                  "Underlying type does not have an array specialization.");

    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr GC::AllocFormat Format =
        VM::ArrayTraits<T>::ArrayFormat;

    // All constructor signatures for Array take the length as the
    // first argument.
    template <typename... Args>
    static uint32_t SizeOf(uint32_t len, Args... rest) {
        return len * sizeof(T);
    }
};

template <>
struct AllocFormatTraits<AllocFormat::AllocThingPointerArray>
{
    AllocFormatTraits() = delete;
    typedef VM::Array<AllocThing *> Type;
};

template <>
struct TraceTraits<VM::Array<GC::AllocThing *>>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;

    static constexpr bool IsLeaf = false;

    typedef VM::Array<GC::AllocThing *> Array_;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const Array_ &array,
                     const void *start, const void *end)
    {
        // Scan each pointer in the array.
        for (uint32_t i = 0; i < array.length(); i++)
            array.vals_[i].scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, Array_ &array,
                       const void *start, const void *end)
    {
        // Update each pointer in the array.
        for (uint32_t i = 0; i < array.length(); i++)
            array.vals_[i].update(updater, start, end);
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

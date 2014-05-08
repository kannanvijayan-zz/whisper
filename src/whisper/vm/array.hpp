#ifndef WHISPER__VM__ARRAY_HPP
#define WHISPER__VM__ARRAY_HPP


#include "common.hpp"
#include "debug.hpp"
#include "spew.hpp"
#include "slab.hpp"
#include "runtime.hpp"

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
    inline Array(uint32_t len, const T *vals) {
        for (uint32_t i = 0; i < len; i++)
            new (&vals_[i]) T(vals[i]);
    }

    inline Array(uint32_t len, const T &val) {
        for (uint32_t i = 0; i < len; i++) {
            SpewMemoryWarn("Array construct %p %d/%d = %d",
                           this, i, len, int(val));
            vals_[i].init(this, val);
            // new (&vals_[i]) T(val);
        }
        SpewMemoryWarn("Array length %d", (int)length());
    }

    inline uint32_t length() const;

    inline const T &getRaw(uint32_t idx) const;
    inline T &getRaw(uint32_t idx);

    inline T get(uint32_t idx) const;

    inline void set(uint32_t idx, const T &val);
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

    typedef VM::Array<GC::AllocThing *> T_;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const T_ &t,
                     const void *start, const void *end)
    {
        // TODO: implement me.
    }

    template <typename Updater>
    static void Update(Updater &updater, T_ &t, void *start, void *end) {
        // TODO: implement me.
    }
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__ARRAY_HPP

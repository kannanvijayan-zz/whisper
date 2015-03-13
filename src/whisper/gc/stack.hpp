#ifndef WHISPER__GC__STACK_HPP
#define WHISPER__GC__STACK_HPP

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"

#include "gc/formats.hpp"

//
// STACK-ALLOCATED STRUCTURES
// ==========================
//
// For stack-allocated structures, the GC needs to do the following:
//
//  a. Traverse a list of all stack-allocated structures.
//  b. Scan structures for references to heap objects.
//  c. Update references to heap objects which have moved.
//
// Note that the GC has to do the traversal dynamically, and thus
// has no static information about the structure of the instances
// being traversed.  It needs to derive the structure of each item
// from the pointer.
//
// To support this, traced types allocated on stack must be wrapped
// with Local<>, e.g.:
//
//      Local<VM::String *> stringPtr(cx, str);
//
// The Local<> instance uses |cx| to add itself to a linked-list of
// all traced types on stack on construction, and remove itself on
// destruction.  C++ RAII semantics guarantees that the destructed local
// will always be the last entry in the linked list.
//
// Values which are held by Local<T> wrappers must have some type T
// which has a specialization for StackTraits<T>.  StackTraits<T> informs
// the GC on how to map T to an StackFormat enum value.
//
// An StackFormat can be mapped (using StackFormatTraits) to a type that
// allows for tracing the allocated structure.
//

namespace Whisper {


class StackThing;

enum class StackFormat : uint16_t
{
    INVALID = 0,
#define ENUM_(T) T,
    WHISPER_DEFN_GC_STACK_FORMATS(ENUM_)
#undef ENUM_
    LIMIT
};

inline constexpr bool IsValidStackFormat(StackFormat fmt) {
    return (fmt > StackFormat::INVALID) && (fmt < StackFormat::LIMIT);
}
inline constexpr uint16_t StackFormatValue(StackFormat fmt) {
    return static_cast<uint16_t>(fmt);
}
const char *StackFormatString(StackFormat fmt);


// StackHeader
// -----------
//
//  An 8-byte (two 32-bit words) structure that specifies the
//  size of a stack-allocated structure, its heap format, and
//  a few bits of type-specific data.
//
//  The High 32-bit word of the StackHeader is simply the size of the
//  allocation in bytes.
//
//  The Low 32-bit word contains the StackFormat in the low 10 bits.
//
//                                StackFormat
//                                  10 bits
//                                    |
//                                    |
//                               ----------
//
//      00000000 00000000 000000 FFFFFFFFFF
//      bit 31                        bit 0
//
//
//      The low 10 bits indicates StackFormat.
class alignas(8) StackHeader
{
  private:
    uint32_t header_;
    uint32_t size_;

    typedef Bitfield<uint32_t, uint16_t, 10, 0> FormatBitfield;

    inline FormatBitfield formatBitfield() {
        return FormatBitfield(header_);
    }
    inline FormatBitfield::Const formatBitfield() const {
        return FormatBitfield::Const(header_);
    }

  public:
    StackHeader(StackFormat fmt, uint32_t size)
      : header_(StackFormatValue(fmt)),
        size_(size)
    {}

    inline StackFormat format() const {
        return static_cast<StackFormat>(formatBitfield().value());
    }
    inline const char *formatString() const {
        return StackFormatString(format());
    }
    inline uint32_t size() const {
        return size_;
    }

    const void *payload() const {
        return reinterpret_cast<const uint8_t *>(this) + sizeof(StackHeader);
    }
    void *payload() {
        return reinterpret_cast<uint8_t *>(this) + sizeof(StackHeader);
    }

#define METHOD_(fmt) \
    bool isFormat_##fmt() const { \
        return format() == StackFormat::fmt; \
    }
    WHISPER_DEFN_GC_STACK_FORMATS(METHOD_)
#undef METHOD_

};


// StackTraits
// -----------
//
//  StackTraits contains the following definitions:
//
//      // Must be set by all StackTraits specializations.
//      static constexpr bool Specialized = true;
//
//      // The StackFormat for the type.
//      static constexpr StackFormat Format;
template <typename T>
struct StackTraits
{
    StackTraits() = delete;

    static constexpr bool Specialized = false;
    // static constexpr StackFormat Format = StackFormat::INVALID;
};


// StackFormatTraits
// -----------------
//
// A template specialization that, for every StackFormat enum value,
// defines the static type that handles it.
//
// This allows the garbage collector to take a pointer to a value,
// obtain its StackFormat value FMT, use StackFormatTraits<FMT>::Type
// to obtain a type T for that format, and then use TraceTraits<T>
// to be able to scan and update the structure.
//
template <StackFormat FMT>
struct StackFormatTraits
{
    StackFormatTraits() = delete;

    // All specializations for FMT must define TYPE to be the
    // static type for the format.  The defined TYPE must have
    // a specialization for TraceTraits<TYPE>.
    //
    // typedef ... Type;
};

// BaseStackTypeTraits
// ------------------
//
// BaseStackTypeTraits specializations annotate a type as having pointers
// which are convertible to a StackThing pointer, even if it does not have
// a StackTraits specialization.  This is useful to enable traced manipulation
// of pointers to base types of traced heap types.
//
template <typename T>
struct BaseStackTypeTraits
{
    BaseStackTypeTraits() = delete;
    static constexpr bool Specialized = false;
};

// StackThing is implicitly specialized for BaseStackTypeTraits.
template <>
struct BaseStackTypeTraits<StackThing>
{
    BaseStackTypeTraits() = delete;
    static constexpr bool Specialized = true;
};

template <typename T>
constexpr inline bool IsStackThingType() {
    return StackTraits<T>::Specialized ||
           BaseStackTypeTraits<T>::Specialized;
}

// StackThing
// ----------
//
// An StackThing is a strucure allocated in memory and directly preceded
// by an StackHeader.
//
// This is a model type, and not intended to participate in an inheritance
// graph.  StackThing::From can be used to create StackThing pointers from
// pointers to types which are StackTraits or BaseStackTypeTraits specialized.
//
// Pointer types are assumed for tracing purposes to be pointers to
// StackThings.
//
class StackThing
{
  private:
    StackThing() = delete;

  public:
    template <typename T>
    static inline StackThing *From(T *ptr) {
        static_assert(IsStackThingType<T>(), "T is not StackThingType.");
        return reinterpret_cast<StackThing *>(ptr);
    }
    template <typename T>
    static inline const StackThing *From(const T *ptr) {
        static_assert(IsStackThingType<T>(), "T is not StackThingType.");
        return reinterpret_cast<const StackThing *>(ptr);
    }

    template <typename T>
    inline T *to() {
        static_assert(IsStackThingType<T>(), "T is not StackThingType.");
        return reinterpret_cast<T *>(this);
    }
    template <typename T>
    inline const T *to() const {
        static_assert(IsStackThingType<T>(), "T is not StackThingType.");
        return reinterpret_cast<const T *>(this);
    }

    inline StackHeader &header() {
        return reinterpret_cast<StackHeader *>(this)[-1];
    }
    inline const StackHeader &header() const {
        return reinterpret_cast<const StackHeader *>(this)[-1];
    }

    inline uint32_t size() const {
        return header().size();
    }

    inline const void *end() const {
        return reinterpret_cast<const uint8_t *>(this) + size();
    }
    inline void *end() {
        return reinterpret_cast<uint8_t *>(this) + size();
    }

    inline StackFormat format() const {
        return header().format();
    }

    // define is
#define CHK_(name) \
    inline bool is##name() const { \
        return format() == StackFormat::name; \
    }
    WHISPER_DEFN_GC_STACK_FORMATS(CHK_)
#undef CHK_
};


} // namespace Whisper

#endif // WHISPER__GC__STACK_HPP

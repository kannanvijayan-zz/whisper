#ifndef WHISPER__GC__HEAP_THINGS_HPP
#define WHISPER__GC__HEAP_THINGS_HPP

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"

#include "gc/formats.hpp"

//
// HEAP-ALLOCATED STRUCTURES
// =========================
//
// For heap-allocated structures, the GC needs to do the following:
//
//  a. Scan a heap-allocated object for references to heap objects.
//  b. Update references to heap objects which have moved.
//  c. When mutating references held by the object, trigger the
//     appropriate pre and post barriers.
//
// Once again, as with stack-allocated structures - the GC needs
// to be able to scan a memory region for its references without
// any a-priori knowledge of its structure.
//
// All heap allocated structures are preceded by two 32-bit words that
// describe the object's size, and a number of extra attributes.  One
// of these attributes is the HeapFormat enum value for the object.
// These two words are modeled by the |HeapHeader| class below.
//
// To enable filling of the HeapFormat when an object is constructed,
// all types T which are allocated on the heap must have specialization
// for HeapTraits<T>.  HeapTraits<T> allows the heap-object instantiator
// to map the type to a HeapFormat that is then stored in the HeapHeader
// for the object.
//
// A HeapFormat can dynamically be mapped (using HeapFormatTraits) back to
// a type that allows for tracing the heap-allocated structure.
//

namespace Whisper {


class HeapThing;


enum class HeapFormat : uint16_t
{
    INVALID = 0,
#define ENUM_(T) T,
    WHISPER_DEFN_GC_HEAP_FORMATS(ENUM_)
#undef ENUM_
    LIMIT
};

inline constexpr bool IsValidHeapFormat(HeapFormat fmt) {
    return (fmt > HeapFormat::INVALID) && (fmt < HeapFormat::LIMIT);
}
inline constexpr uint16_t HeapFormatValue(HeapFormat fmt) {
    return static_cast<uint16_t>(fmt);
}
char const* HeapFormatString(HeapFormat fmt);


// GC Generation
enum class Gen : uint8_t
{
    None        = 0x00,

    Hatchery    = 0x01,
    Nursery     = 0x02,
    Mature      = 0x03,
    Tenured     = 0x04,

    Immortal    = 0x05,

    LIMIT
};
static constexpr bool IsValidGen(Gen gen) {
    return (gen >= Gen::None) && (gen < Gen::LIMIT);
}
char const* GenString(Gen gen);

// HeapHeader
// ----------
//
//  An 8-byte (two 32-bit words) structure that specifies the
//  size of a heap-allocated structure, its generation, its
//  heap format, and a few bits of type-specific data.
//
//
//  The High 32-bit word of the HeapHeader is simply the size of the
//  allocation in bytes.
//
//  The Low 32-bit word of the HeapHeader is formatted as follows:
//
//              Generation
//               3 bits
//                  |
//      HeapFormat  |      UserData  CardNo
//        10 bits   |       8 bits   10 bits
//           |      |        |         |
//           |      |        |         |
//      ---------- ---   -------- ----------
//
//      FFFFFFFFFF GGG 0 UUUUUUUU CCCCCCCCCC
//      bit 31                          bit 0
//
//
//      The low 10 bits indicates the card this object lives on.
//
//      The next 6 bits are "userdata", free for use by specific formats
//      to stow away a few state bits without needing to use an extra
//      word in the object.
//
//      The next 2 bits are zero.
//
//      The next 3 bits specify the GC value for the structure,
//      which indicates its allocation site.
//
//      The high 10 bits specify the HeapFormat for the structure.
//
class alignas(8) HeapHeader
{
  private:
    uint32_t header_;
    uint32_t size_;

    typedef Bitfield<uint32_t, uint16_t, 10, 0> CardBitfield;
    typedef Bitfield<uint32_t, uint8_t, 8, 10> UserBitfield;
    typedef Bitfield<uint32_t, uint8_t, 3, 19> GcBitfield;
    typedef Bitfield<uint32_t, uint16_t, 10, 22> FormatBitfield;

    static_assert(static_cast<uint8_t>(Gen::LIMIT) <= GcBitfield::MaxValue,
                  "Gen must fit within 3 bits.");

    inline CardBitfield cardBitfield() {
        return CardBitfield(header_);
    }
    inline CardBitfield::Const cardBitfield() const {
        return CardBitfield::Const(header_);
    }
    inline FormatBitfield formatBitfield() {
        return FormatBitfield(header_);
    }
    inline FormatBitfield::Const formatBitfield() const {
        return FormatBitfield::Const(header_);
    }
    inline UserBitfield userBitfield() {
        return UserBitfield(header_);
    }
    inline UserBitfield::Const userBitfield() const {
        return UserBitfield::Const(header_);
    }
    inline GcBitfield gcBitfield() {
        return GcBitfield(header_);
    }
    inline GcBitfield::Const gcBitfield() const {
        return GcBitfield::Const(header_);
    }

  public:
    static constexpr uint16_t CardMax = CardBitfield::MaxValue;
    static constexpr uint8_t UserDataMax = UserBitfield::MaxValue;

  public:
    HeapHeader(HeapFormat fmt, Gen gen, uint16_t card, uint32_t size)
      : header_(0),
        size_(size)
    {
        cardBitfield().initValue(card);
        formatBitfield().initValue(static_cast<uint8_t>(fmt));
        gcBitfield().initValue(static_cast<uint8_t>(gen));
    }

    inline HeapFormat format() const {
        return static_cast<HeapFormat>(formatBitfield().value());
    }
    inline char const* formatString() const {
        return HeapFormatString(format());
    }
    inline uint16_t card() const {
        return cardBitfield().value();
    }
    inline uint32_t size() const {
        return size_;
    }

    inline Gen gen() const {
        return static_cast<Gen>(gcBitfield().value());
    }
    inline char const* genString() const {
        return GenString(gen());
    }
    inline bool inGen(Gen g) const {
        return gen() == g;
    }

    inline uint8_t userData() const {
        return userBitfield().value();
    }
    inline void initUserData(uint8_t val) {
        userBitfield().initValue(val);
    }
    inline void setUserData(uint8_t val) {
        userBitfield().setValue(val);
    }

    void const* payload() const {
        return reinterpret_cast<uint8_t const*>(this) + sizeof(HeapHeader);
    }
    void* payload() {
        return reinterpret_cast<uint8_t*>(this) + sizeof(HeapHeader);
    }

#define METHOD_(fmt) \
    bool isFormat_##fmt() const { \
        return format() == HeapFormat::fmt; \
    }
    WHISPER_DEFN_GC_HEAP_FORMATS(METHOD_)
#undef METHOD_

};

// HeapTraits
// ----------
//
//  HeapTraits contains the following definitions:
//
//      // Must be set by all StackTraits specializations.
//      static constexpr bool Specialized = true;
//
//      // The HeapFormat for the type.
//      static constexpr HeapFormat Format;
//
//      // Indicates whether the type is variable sized.
//      static constexpr bool VarSized;
//
template <typename T>
struct HeapTraits
{
    HeapTraits() = delete;

    static constexpr bool Specialized = false;

    // static constexpr HeapFormat Format;
    // static constexpr bool VarSized;
};


// HeapFormatTraits
// -----------------
//
// A template specialization that, for every HeapFormat enum value,
// defines the static type that handles it.
//
// This allows the garbage collector to take a pointer to a value,
// obtain its HeapFormat value FMT, use HeapFormatTraits<FMT>::Type
// to obtain a type T for that format, and then use TraceTraits<T>
// to be able to scan and update the structure.
//
template <HeapFormat FMT>
struct HeapFormatTraits
{
    HeapFormatTraits() = delete;

    // typedef ... Type;
};

// BaseHeapTypeTraits
// ------------------
//
// BaseHeapTypeTraits specializations annotate a type as having pointers
// which are convertible to a HeapThing pointer, even if it does not have
// a HeapTraits specialization.  This is useful to enable traced manipulation
// of pointers to base types of traced heap types.
//
template <typename T>
struct BaseHeapTypeTraits
{
    BaseHeapTypeTraits() = delete;
    static constexpr bool Specialized = false;
};

// HeapThing is implicitly specialized for BaseHeapTypeTraits.
template <>
struct BaseHeapTypeTraits<HeapThing>
{
    BaseHeapTypeTraits() = delete;
    static constexpr bool Specialized = true;
};

template <typename T>
constexpr inline bool IsHeapThingType() {
    return HeapTraits<T>::Specialized ||
           BaseHeapTypeTraits<T>::Specialized;
}


// HeapThing
// ---------
//
// An HeapThing is a strucure allocated in memory and directly preceded
// by an HeapHeader.
//
// This is a model type, and not intended to participate in an inheritance
// graph.  HeapThing::From can be used to create HeapThing pointers from
// pointers to types which are HeapTraits or HeapThingTraits specialized.
//
// Pointer types are assumed for tracing purposes to be pointers to
// HeapThings.
//
class HeapThing
{
  private:
    HeapThing() = delete;

  public:
    template <typename T>
    static inline HeapThing* From(T* ptr) {
        static_assert(IsHeapThingType<T>(), "T is not HeapThingType.");
        return reinterpret_cast<HeapThing*>(ptr);
    }
    template <typename T>
    static inline HeapThing const* From(T const* ptr) {
        static_assert(IsHeapThingType<T>(), "T is not HeapThingType.");
        return reinterpret_cast<HeapThing const*>(ptr);
    }

    template <typename T>
    inline T* to() {
        static_assert(IsHeapThingType<T>(), "T is not HeapThingType.");
        return reinterpret_cast<T*>(this);
    }
    template <typename T>
    inline T const* to() const {
        static_assert(IsHeapThingType<T>(), "T is not HeapThingType.");
        return reinterpret_cast<T const*>(this);
    }

    inline HeapHeader& header() {
        return reinterpret_cast<HeapHeader*>(this)[-1];
    }
    inline HeapHeader const& header() const {
        return reinterpret_cast<HeapHeader const*>(this)[-1];
    }

    inline uint32_t size() const {
        return header().size();
    }

    inline void const* end() const {
        return reinterpret_cast<uint8_t const*>(this) + size();
    }
    inline void* end() {
        return reinterpret_cast<uint8_t*>(this) + size();
    }

    inline HeapFormat format() const {
        return header().format();
    }

    inline uint8_t userData() const {
        return header().userData();
    }

    inline Gen gen() const {
        return header().gen();
    }

    // define is
#define CHK_(name) \
    inline bool is##name() const { \
        return format() == HeapFormat::name; \
    }
    WHISPER_DEFN_GC_HEAP_FORMATS(CHK_)
#undef CHK_
};


} // namespace Whisper

#endif // WHISPER__GC__HEAP_THINGS_HPP

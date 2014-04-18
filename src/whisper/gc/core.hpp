#ifndef WHISPER__GC__CORE_HPP
#define WHISPER__GC__CORE_HPP

#include "common.hpp"
#include "debug.hpp"

//
// The garbage collector needs to inspect the following things about
// the runtime:
//
//  1. The stack, and all references from the stack to objects.
//
//  2. Given a pointer to a AllocThing, a way of scanning it for
//     references to the heap (possibly encoded, e.g. tagged pointers),
//     and updating those references when heap values move.
//
//  Every AllocThing pointer needs to be convertible to a pointer to
//  a type for which TraceTraits is specialized.  TraceTraits defines,
//  among other things, a SCAN method and an UPDATE method.
//
//
// PURPOSE 1: Identify all stack-rooted objects.
//
//  All stack values which may embed references to other
//  objects must be rooted using |Local|.  A |Local<T>|
//  is constructed with the current thread context.  It inherits
//  from LocalBase, an untyped base class which forms a linked
//  list that hangs off of the thread context.
//
//  This linked list can be traversed to obtain pointers to all
//  stack rooted values.
//
//  This still leaves the issue of scanning these values for
//  embedded pointers.  For this, the address is not sufficient,
//  because the value may embed multiple pointers, or embed tagged
//  pointers.  This is addressed later.
//
// PURPOSE 2: Scan AllocThings for references, and update them.
//
//  All AllocThings are allocated as 8-byte aligned pointers with a
//  preceding header word, which contains the AllocFormat for the
//  structure.
//
//  The AllocFormatTraits template is used to map an AllocFormat FMT
//  to a type which can be used to specialized TraceTraits, via
//  AllocFormatTraits<FMT>::TYPE.
//
//  Then, the methods TraceTraits<T>::SCAN and TraceTraits<T>::UPDATE are used
//  to scan the object for heaprefs, and to update those heaprefs with new
//  pointers respectively.
//

namespace Whisper {


#define WHISPER_DEFN_GC_ALLOC_FORMATS(_) \
    _(UntracedThing)        \
    _(TracedPointer)

enum class AllocFormat : uint16_t
{
    INVALID = 0,
#define ENUM_(T) T,
    WHISPER_DEFN_GC_ALLOC_FORMATS(ENUM_)
#undef ENUM_
    LIMIT
};

inline constexpr uint32_t AllocFormatValue(AllocFormat fmt) {
    return static_cast<uint32_t>(fmt);
}

inline constexpr bool IsValidAllocFormat(AllocFormat fmt) {
    return (fmt > AllocFormat::INVALID) && (fmt < AllocFormat::LIMIT);
}

// GC Generation
enum class GCGen : uint8_t {
    None        = 0x00,
    OnStack     = 0x01,
    LocalHeap   = 0x02,
    Hatchery    = 0x03,
    Nursery     = 0x04,
    Mature      = 0x05,
    Tenured     = 0x06
};

class alignas(8) AllocHeader
{
  private:
    //
    //  +---------------------------------------------------+
    //  | FRMT:10 | GC:3 | 000 | USR:6  | CARD:10 | SIZE:32 |
    //  +---------------------------------------------------+
    //
    //  The high 32 bits of the header indicates the object's size.
    //
    //  The low 32 bits are organized as follows:
    //
    //      The first 10 bits indicates the card this object lives on.
    //
    //      The next 6 bits are "userdata", free for use by specific formats
    //      to stow away a few state bits without needing to use an extra
    //      word in the object.
    //
    //      The next 3 bits are zero.
    //
    //      The next 3 bits specify the GC value for the structure,
    //      which indicates its allocation site.
    //
    //      The next 10 bits specify the AllocFormat for the structure.
    //
    // Low word is format, flags, and card; high word is size.
    uint32_t header_;
    uint32_t size_;

    typedef Bitfield<uint32_t, uint16_t, 10, 0> CardBitfield;
    typedef Bitfield<uint32_t, uint8_t, 6, 10> UserBitfield;
    typedef Bitfield<uint32_t, uint8_t, 3, 19> GcBitfield;
    typedef Bitfield<uint32_t, uint16_t, 10, 22> FormatBitfield;

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
    AllocHeader(AllocFormat fmt, GCGen gen, uint16_t card, uint32_t size)
      : header_(0),
        size_(size)
    {
        cardBitfield().initValue(card);
        formatBitfield().initValue(static_cast<uint8_t>(fmt));
        gcBitfield().initValue(static_cast<uint8_t>(gen));
    }

    inline AllocFormat format() const {
        return static_cast<AllocFormat>(formatBitfield().value());
    }
    inline uint16_t card() const {
        return cardBitfield().value();
    }
    inline uint32_t size() const {
        return size_;
    }

    inline GCGen gcGen() const {
        return static_cast<GCGen>(gcBitfield().value());
    }
    inline bool isOnStack() const {
        return gcGen() == GCGen::OnStack;
    }
    inline bool isLocalHeap() const {
        return gcGen() == GCGen::LocalHeap;
    }
    inline bool isHatchery() const {
        return gcGen() == GCGen::Hatchery;
    }
    inline bool isNursery() const {
        return gcGen() == GCGen::Nursery;
    }
    inline bool isMature() const {
        return gcGen() == GCGen::Mature;
    }
    inline bool isTenured() const {
        return gcGen() == GCGen::Tenured;
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

    const void *payload() const {
        return reinterpret_cast<const uint8_t *>(this) + sizeof(AllocHeader);
    }
    void *payload() {
        return reinterpret_cast<uint8_t *>(this) + sizeof(AllocHeader);
    }
};


//
// AllocTraits<T>
//
// A template specialization that defines various attributes of the type.
//
// AllocTraits must be specialized for any object intended for use
// with Local, or for allocation on slabs.
//
template <typename T>
struct AllocTraits
{
    AllocTraits() = delete;

    // All specializations for T must set SPECIALIZED to true.
    static constexpr bool SPECIALIZED = false;

    // All specializations for T must define FORMAT to an appropriate
    // AllocFormat.
    //
    // static constexpr AllocFormat FORMAT = ...;

    //
    // typedef ... DEREF_TYPE;
    // static DEREF_TYPE *DEREF(T &t);
    // static const DEREF_TYPE *DEREF(const T &t);
    //      When a value of the specialized type is being held by a container
    //      class, it is convenient to call methods defined on the type directly
    //      from the holder (Local<T> or Field<T>).  These holders overload
    //      the '->' operator to forward to the underlying type.  However, some
    //      types (such as pointers-to-slabthings) may want to forward method
    //      calls through the pointer.  To allow types to choose how to expose
    //      methods, the DEREF method can be specialized to return the right
    //      pointer given a reference to the value.
};


//
// AllocFormatTraits<F>
//
// A template specialization that, for every AllocFormat enum value,
// defines the static type that handles it.
//
// This allows the garbage collector to take a pointer to a value,
// obtain its AllocFormat value FMT, use AllocFormatTraits<FMT>::TYPE
// to obtain a type T for that format, and then use TraceTraits<T>
// to be able to scan and update the structure.
//
template <AllocFormat FMT>
struct AllocFormatTraits
{
    AllocFormatTraits() = delete;

    // All specializations for FMT must define TYPE to be the
    // static type for the format.  The defined TYPE must have
    // a specialization for TraceTraits<TYPE>.
    //
    // typedef ... TYPE;

    // All specializations of FMT must define a static constexpr
    // boolean field indicating if the type needs to be traced
    // or not.  If instances of the type are guranteed never to
    // hold any references into the heap, this property can
    // be set to true and objects of that format will never be
    // scanned for references.
    //
    // This field also influences which region of a slab instances
    // are allocated into.  Traced and untraced objects are segregated
    // in slabs so that references are more packed together in memory.
    //
    // static constexpr bool TRACED = ...;
};


//
// TraceTraits<T>()
//
// Specializations of TraceTraits must be provided by types before any
// gc-aware containers can be used to host them.
//
// The specializations must define:
//
//  static constexpr bool SPECIALIZED = true;
//      Marker boolean indicating that TraceTraits has been specialized
//      for this type.
//
//  template <typename Scanner>
//  void SCAN(Scanner &scanner, const T &t, void *start, void *end);
//      Scan the a thing of type T for references.
//
//      Assume |scanner| is a callable with the following signature:
//
//          void scanner(void *addr, AllocThing *ptr);
//
//      The |scanner| should be called for every reference to an AllocThing
//      contained within T.  For each call, |addr| should be the address
//      of the pointer (i.e. the location in the containing object which
//      holds the pointer), and |ptr| should be the value of the pointer
//      (i.e. the thing being pointed to).
//
//      The |start| and |end| pointers are hints.  If start is non-null
//      then it indicates that pointers that occur before |start| can be
//      ignored.  If end is non-null, then it indicates that pointers that
//      occur after |end| can be ignored.
//
//      For untraced types, this method must be implemented, but can simply
//      be a no-op.
//
//  template <typename Updater>
//  void UPDATE(Updater &updater, T &t, void *start, void *end);
//      Method to update a previously-scanned value with moved pointers.
//
//      Assume |updater| is a callable with the following signature:
//
//          AllocThing *updater(void *addr, AllocThing *ptr);
//
//      The |updater| should be called for every reference to an AllocThing
//      contained within T (as with |SCAN| above).  If the returned pointer
//      is different from |ptr|, then |addr| should be updated to contain
//      the new pointer.
//
//      As with |SCAN|, the |start| and |end| parameters are hints
//      identifying regions of memory which are of interest.
//
//      For untraced types, this method must be implemented, but can simply
//      be a no-op.
//
//
// Note that if the type embeds any Field<...> members, its TRACE method
// must call trace on the embedded values.
//
template <typename T>
struct TraceTraits {
    TraceTraits() = delete;

    static constexpr bool SPECIALIZED = false;

    /*
     * All definitions below are static:
     *
     * template <typename Scanner>
     * static void SCAN(Scanner &scaner, const T &t, void *start, void *end);
     *
     *
     * template <typename Updater>
     * staticvoid UPDATE(Updater &updater, T &t, void *start, void *end);
     *
     */
};


///////////////////////////////////////////////////////////////////////////////

//
// AllocThing
//
// Model type for the base set of structure posessed by AllocThings -
// namely accessing the meta-data in the AllocHeader that precedes
// the AllocThing.
//
class AllocThing
{
  private:
    AllocThing() = delete;

  public:
    template <typename T>
    static inline AllocThing *From(T *ptr) {
        static_assert(AllocTraits<T>::SPECIALIZED,
                      "AllocTraits<T> not specialized.");
        return reinterpret_cast<AllocThing *>(ptr);
    }
    template <typename T>
    static inline const AllocThing *From(const T *ptr) {
        static_assert(AllocTraits<T>::SPECIALIZED,
                      "AllocTraits<T> not specialized.");
        return reinterpret_cast<const AllocThing *>(ptr);
    }

    inline AllocHeader &header() {
        return reinterpret_cast<AllocHeader *>(this)[-1];
    }
    inline const AllocHeader &header() const {
        return reinterpret_cast<const AllocHeader *>(this)[-1];
    }

    inline uint32_t size() const {
        return header().size();
    }

    inline const void *end() const {
        return reinterpret_cast<const uint8_t *>(this) + size();
    }

    inline AllocFormat format() const {
        return header().format();
    }

    inline uint8_t userData() const {
        return header().userData();
    }

    inline GCGen gcGen() const {
        return header().gcGen();
    }

    // define is
#define CHK_(name) \
    inline bool is##name() const { \
        return format() == AllocFormat::name; \
    }
    WHISPER_DEFN_GC_ALLOC_FORMATS(CHK_)
#undef CHK_
};



} // namespace Whisper

#endif // WHISPER__GC__CORE_HPP

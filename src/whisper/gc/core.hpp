#ifndef WHISPER__GC__CORE_HPP
#define WHISPER__GC__CORE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"

#include <type_traits>

//
//
// INTRODUCTION
// ============
//
// Traced structures interact with the GC in three different roles
//
//  1. As a stack-allocated structure.
//
//  2. As a heap-allocated structure.
//
//  3. As a field in a an allocated structure.
//
// A type can serve in more than one of these roles, including all of them.
//
// The interaction of the GC with types playing these different roles
// is different, and requires different information.
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
// The Local<> instance uses |cx| to add themselves to a linked-list of
// all traced types on stack on construction, and remove themselves on
// destruction.  C++ RAII semantics guarantees that the destructed local
// will always be the last entry in the linked list.
//
// Values which are held by Local<T> wrappers must have some type T
// which has a specialization for StackTraits<T>.  StackTraits<T> informs
// the GC on how to map T to an AllocFormat enum value.
//
// An AllocFormat can dynamically be mapped back to a type that allows
// for tracing the allocated structure.  This mapping is described later
// in the TRACING section, because it also used by the heap-structure
// tracing implementation described in HEAP-ALLOCATED STRUCTURES.
//
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
// All HeapAllocated structures are preceded by two 32-bit words that
// describe the object's size, and a number of extra attributes.  One
// of these attributes is the AllocFormat enum value for the object.
// These two words are modeled by the |AllocHeader| class below.
//
// To enable filling of the AllocFormat when an object is constructed,
// all types T which are allocated on the heap must have specialization
// for HeapTraits<T>.  HeapTraits<T> allows the heap-object instantiator
// to map the type to an AllocFormat that is then stored in the AllocHeader
// for the object.
//
// An AllocFormat can dynamically be mapped back to a type that allows
// for tracing the allocated structure.  This mapping is described later
// in the TRACING section, because it also used by the stack-structure
// tracing implementation described in STACK-ALLOCATED STRUCTURES.
//
// FIELD STRUCTURES
// ================
//
// It is useful to be able to embed within traced objects
// types whose instances may hold references to heap objects.
//
// These fields instances must allow for their references to be
// scanned and updated, since they are embedded within
// heap and stack allocated structures.
//
// Since fields are embedded in their containing structure, the
// static type of the containing structure captures the static
// type of the field structure.  The GC will dynamically infer the
// static type of the containing structure (which is either on stack
// or on heap), and thus implicitly infer the static type of the field.
//
// Fields on stack structures behave differently from fields on heap
// structures.  For example, mutations of fields on stack-allocated
// objects require no additional bookkeeping, but mutations of fields
// on heap-allocated objects may trigger write barriers.
//
// To support these two designs, there are two field wrapper templates:
// StackField<T> and HeapField<T>.  StackField<T>s are meant to be contained
// within a container type C allocated in a StackField<C> or Local<C>.
// HeapField<T>s are meant to be contained within a container type C
// allocated in a HeapField<C> or on the heap.
//
// To be able to be wrapped by StackField or HeapField, a type T must
// specialize FieldTraits<T>, and addictionally TraceTraits<T>.  See
// the TRACING section for more details.
//
// TRACING
// =======
//
// To trace stack and heap allocated top-level structures, the GC
// first obtains the AllocFormat of the structure, and then maps this
// format to a type X using AllocFormatTraits<AllocFormat FORMAT>.
//
// The mapped type X is expected to have a specialization TraceTraits<X>.
// TraceTraits defines static methods that express the tracing
// and update of references-to-heap stored within X.
//
// Traced field types wrapped with StackField and HeapField are expected
// to directly specialize TraceTraits.
//
//
// The following diagram describes how various instantiation domains relate
// to the derivation of the tracing and update procedure for as structure:
//
//            +-----------+      +----------+        +-----------+
//            | StackType |      | HeapType |  .---->| FieldType |
//            +-----------+      +----------+  |     +-----------+
//                  |                 |        |
//              via |             via |        |
//       StackTraits|       HeapTraits|        |
//                  |                 |        |           ^
//                  .-----------------.        |           |
//                           |                 |           |
//                           |                 |           |
//                           v                 |           |
//                    +-------------+          |           |
//                    | AllocFormat |          |           |
//                    +-------------+          |           |
//                           |                 |           |
//                       via |                 |           |
//          AllocFormatTraits|                 |           |
//                           |                 |           |
//                           v                 |           |
//                    +------------+  embeds   |           |
//                    | TracedType |-----------.           |
//                    +------------+                       |
//                                                         |
//                                                         |
//                                                         |
//                           ^                             |
//                           |                             |
//                           |                             |
//                         Both of these specialize TraceTraits
//
// TRAITS
// ======
//
// The design of the system is to use generic traits templates to
// annotate types and enum values with static information:
//
//  1. struct StackTraits<typename T>
//
//      The StackTraits template indicates that T is suitable for use
//      with Local<> containers.
//
//      A StackTraits specialization must provide the following features:
//
//          static constexpr bool Specialized = true;
//          static constexpr AllocFormat Format;
//
//  2. struct HeapTraits<typename T>
//
//      The HeapTraits template indicates that T is suitable for allocation
//      on the heap.
//
//      A HeapTraits specialization must provide the following features:
//
//          static constexpr bool Specialized = true;
//          static constexpr AllocFormat Format;
//          static constexpr bool VarSized;
//
//      VarSized indicates that the underlying type may have variable
//      size, and thus must be allocated only using methods which
//      explicitly specify its size.
//
//  3. struct FieldTraits<T>
//
//      The FieldTraits template just marks the type as usable as
//      a field.
//
//      Types which specialize FieldTraits<T> must also specialize
//      TraceTraits<T>.  The TraceTraits specialization for this type
//      cannot rely on being able to access the AllocHeader header word.
//
//      A FieldTraits specialization must provide the following features:
//
//          static constexpr bool Specialized = true;
//
//  4. enum AllocFormat
//
//      An AllocFormat is an enum value that can be mapped to a
//      type which exposes the structure of an instance in memory.
//      A giant switch statement can be used to generate a dispatch
//      method that calls the correct code given a value of AllocFormat
//      at runtime.
//
//  5. AllocFormatTraits<Format>
//
//      Maps an AllocFormat value to a type which is traceable.
//
//      An AllocFormatTraits specialization must exist for every AllocFormat
//      value, and provide the following features for each:
//
//              typedef TT Type;
//
//      where TT must have a specialization TraceTraits<TT>.
//
//  4. TraceTraits<TT>
//
//      Specifies tracing behaviour for a traceable type.
//      See documentation in the class for the interface it must
//      present.
//
//  5. DerefTraits<T>
//
//      DerefTraits is an optional template-specialization.  There
//      is a default provided that works for all types, but individual
//      types may override the default behaviour.
//
//      DerefTraits exists to solve a syntactic hygiene issue with
//      using template-containers for handlign traced objects.  It is
//      convenient to directly access the API of the underlying
//      traced type without having to unwrap it.
//
//      It would be unfortunate to have to do:
//
//          Local<T> t;
//          t.get().someMethod();
//
//      To avoid this, the wrapper templates override the arrow
//      operator to forward methods to the underlying type.  However,
//      some types may need swizzling to obtain a proper dispatch type
//      for an API.
//
//      Let's say we have a default-behaviour type Pair which is
//      used with Local:
//
//          Local<Pair> pair;
//
//      The default behaviour is for the arrow method on Local to forward
//      calls to the underlying object.  |pair->first()| would be equivalent
//      to |pair.get().first()|.
//
//      But consider a Local-wrapped pointer to a pair allocated on the heap:
//
//          Local<Pair *> ptr;
//
//      The default behaviour does not work here.  |ptr->first()| would be
//      equivalent to |ptr.get().first()|, but the return type of
//      |ptr.get()| is a pointer, and pointers do not have any direct
//      methods.  For pointers, the arrow forwarding should go to the
//      target pointed-to-object, not the pointer value itself.
//
//      DerefTraits solves this problem by allowing types to be mapped
//      to a deref type, and providing a static method to obtain
//      deref type pointers from a reference to the original type.
//
//      DerefTraits<T> specializations must provide the following features:
//
//          typedef DT Type;
//
//          static const DT *Deref(const T &t);
//
//          static DT *Deref(T &t);
//
//      This allows mapping from T to a deref type DT, and furthemore
//      extraction of a pointer to DT from a reference to T.
//

namespace Whisper {
namespace GC {

class AllocThing;

#define WHISPER_DEFN_GC_ALLOC_FORMATS(_) \
    _(UntracedThing)            \
    _(String)                   \
    _(Box)                      \
    _(BoxArray)                 \
    _(PackedSyntaxTree)         \
    _(PackedWriter)             \
    _(AllocThingPointer)        \
    _(AllocThingPointerArray)   \
    _(Vector)                   \
    _(AllocThingPointerVectorContents) \
    _(RootShype)                \
    _(AddSlotShype)             \
    _(SourceFile)

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

const char *AllocFormatString(AllocFormat fmt);

inline constexpr bool IsValidAllocFormat(AllocFormat fmt) {
    return (fmt > AllocFormat::INVALID) && (fmt < AllocFormat::LIMIT);
}

// GC Generation
enum class Gen : uint8_t {
    None        = 0x00,

    // Stack and local allocations.
    OnStack     = 0x01,
    LocalHeap   = 0x02,

    // Heap Allocations.
    Hatchery    = 0x03,
    Nursery     = 0x04,
    Mature      = 0x05,
    Tenured     = 0x06,

    LIMIT
};

class alignas(8) AllocHeader
{
  private:
    //
    //
    //  The High 32-bit word of the AllocHeader is simply the size of the
    //  allocation in bytes.
    //
    //  The Low 32-bit word of the AllocHeader is formatted as follows:
    //
    //              Generation
    //               3 bits
    //                  |
    //      AllocFormat |      UserData  CardNo
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
    //      The high 10 bits specify the AllocFormat for the structure.
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
    AllocHeader(AllocFormat fmt, Gen gen, uint16_t card, uint32_t size)
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
    inline const char *formatString() const {
        return AllocFormatString(format());
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
    inline bool isOnStack() const {
        return gen() == Gen::OnStack;
    }
    inline bool isLocalHeap() const {
        return gen() == Gen::LocalHeap;
    }
    inline bool isHatchery() const {
        return gen() == Gen::Hatchery;
    }
    inline bool isNursery() const {
        return gen() == Gen::Nursery;
    }
    inline bool isMature() const {
        return gen() == Gen::Mature;
    }
    inline bool isTenured() const {
        return gen() == Gen::Tenured;
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

#define METHOD_(fmt) \
    bool isFormat_##fmt() const { \
        return format() == AllocFormat::fmt; \
    }
    WHISPER_DEFN_GC_ALLOC_FORMATS(METHOD_)
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
//      // The AllocFormat for the type.
//      static constexpr AllocFormat Format;
template <typename T>
struct StackTraits
{
    StackTraits() = delete;

    static constexpr bool Specialized = false;
    static constexpr AllocFormat Format = AllocFormat::INVALID;
};

// HeapTraits
// ----------
//
//  HeapTraits contains the following definitions:
//
//      // Must be set by all StackTraits specializations.
//      static constexpr bool Specialized = true;
//
//      // The AllocFormat for the type.
//      static constexpr AllocFormat Format;
//
//      // Methods to calculate the size of the object, given
//      // constructor arguments.
//      static constexpr bool VarSized;
//      
template <typename T>
struct HeapTraits
{
    HeapTraits() = delete;

    static constexpr bool Specialized = false;

    // static constexpr AllocFormat Format;
    // static constexpr bool VarSized;
};

// FieldTraits
// -----------
//
//  FieldTraits contains the following definitions:
//
//      // Must be set true by all FieldTraits specializations.
//      static constexpr bool Specialized = true;
template <typename T>
struct FieldTraits
{
    FieldTraits() = delete;

    static constexpr bool Specialized = false;
};


// AllocFormatTraits
// -----------------
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
    // typedef ... Type;
};


// TraceTraits
// -----------
//
// Specializations of TraceTraits must be provided by types before any
// gc-aware containers can be used to host them.
//
// The specializations must define:
//
//  static constexpr bool Specialized = true;
//      Marker boolean indicating that TraceTraits has been specialized
//      for this type.
//
//      // Indicate if the type needs to be traced or not.
//      // If a type is a leaf type, its values can never contain
//      // pointers.
//      // This will determine if the object is allocated in
//      // the head or tail region of the slab.
//      static constexpr bool IsLeaf;
//
//  template <typename Scanner>
//  void Scan(Scanner &scanner, const T &t, void *start, void *end);
//      Scan the a thing of type T for references.
//
//      Assume |scanner| is a callable with the following signature:
//
//          void scanner(const void *addr, AllocThing *ptr);
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
//  void Update(Updater &updater, T &t, void *start, void *end);
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
//      As with |Scan|, the |start| and |end| parameters are hints
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
struct TraceTraits
{
    TraceTraits() = delete;

    static constexpr bool Specialized = false;

    // Indicate if the type needs to be traced or not.
    // If a type is a leaf type, its values can never contain
    // pointers.
    // This will determine if the object is allocated in
    // the head or tail region of the slab.
    // static constexpr bool IsLeaf = false;

    // template <typename Scanner>
    // static void Scan(Scanner &scanner, const T &t,
    //                  const void *start, const void *end);

    // template <typename Updater>
    // static void Update(Updater &updater, T &t,
    //                    const void *start, const void *end);
};

// DerefTraits<T>
//
// Often, we wish to use underlying structure's methods on the wrapped
// instances of the methods.  For example, we may want to call
//      local->foo()
// where |foo| is a method defined on the underlying type held by the
// Local<T> wrapper.  A type may want to expose a different API through
// its deref traits than it provides directly.
//
// For example, a pointer type would presumably prefer that methods be
// dipatched on the base type of the pointer rather than the pointer type
// itself.
//
// To support this, DerefTraits is used by the GC container classes to
// convert an instance of T into another type that can be dispatched on.
//
// By default, DerefTraits simply maps T to T.
template <typename T>
struct DerefTraits
{
    DerefTraits() = delete;

    typedef T Type;

    // Given an reference to T, return a pointer to Type.
    static inline const Type *Deref(const T &t) {
        return &t;
    }
    static inline Type *Deref(T &t) {
        return &t;
    }
};


///////////////////////////////////////////////////////////////////////////////


//
// AllocThingTraits specializations annotate a type as being convertible
// to an AllocThing pointer, even if it does not have a HeapTraits or
// StackTraits specialization.  This is useful to enable traced manipulation
// of pointers to these base types.
//
template <typename T>
struct AllocThingTraits
{
    AllocThingTraits() = delete;
    static constexpr bool Specialized = false;
};

// AllocThing automatically gets a specialization for
// AllocThingTraits.
template <>
struct AllocThingTraits<AllocThing>
{
    AllocThingTraits() = delete;
    static constexpr bool Specialized = true;
};

//
// UntracedType
//
// A well-understood gc-traced type that contains no pointers.
// Mapping to this type in AllocFormatTraits is an easy way
// to specify TraceTraits for leaf-objects.
// 
class UntracedType {};

//
// AllocThing
//
// An AllocThing is a strucure allocated in memory and directly preceded
// by an AllocHeader.
//
// This is a model type, and not intended to participate in an inheritance
// graph.  AllocThing::From can be used to create AllocThing pointers from
// pointers to types which are either StackTraits or HeapTraits specialized.
//
// Pointer types by default are assumed to be pointers to AllocThings.
// They all use AllocFormat::AllocThingPointer as their format.  For
// tracing, AllocFormatTraits maps this format value to the type
// |AllocThing *|.
//
// These specializations can be found in "gc/specializations.hpp"
//
class AllocThing
{
  private:
    AllocThing() = delete;

  public:
    template <typename T>
    static inline AllocThing *From(T *ptr) {
        static_assert(StackTraits<T>::Specialized ||
                      HeapTraits<T>::Specialized ||
                      AllocThingTraits<T>::Specialized,
                      "Neither StackTraits<T> nor HeapTraits<T> specialized.");
        return reinterpret_cast<AllocThing *>(ptr);
    }
    template <typename T>
    static inline const AllocThing *From(const T *ptr) {
        static_assert(StackTraits<T>::Specialized ||
                      HeapTraits<T>::Specialized ||
                      AllocThingTraits<T>::Specialized,
                      "Neither StackTraits<T> nor HeapTraits<T> specialized.");
        return reinterpret_cast<const AllocThing *>(ptr);
    }
    static inline const AllocThing *From(const AllocThing *ptr) {
        return ptr;
    }
    static inline AllocThing *From(AllocThing *ptr) {
        return ptr;
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
    inline void *end() {
        return reinterpret_cast<uint8_t *>(this) + size();
    }

    inline AllocFormat format() const {
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
        return format() == AllocFormat::name; \
    }
    WHISPER_DEFN_GC_ALLOC_FORMATS(CHK_)
#undef CHK_
};

///////////////////////////////////////////////////////////////////////////////

//
// ScannerBox, ScannerBoxFor<T>, UpdaterBox, and UpdaterBoxFor<T>
// serve to abstract the scanning and update protocol so the actual logic
// can be pushed into a cpp file instead of in a header.
//

class ScannerBox
{
  protected:
    ScannerBox() {}

  public:
    virtual void scanImpl(const void *addr, AllocThing *ptr) = 0;

    inline void operator () (const void *addr, AllocThing *ptr) {
        scanImpl(addr, ptr);
    }
};

template <typename Scanner>
class ScannerBoxFor : public ScannerBox
{
  private:
    Scanner &scanner_;

  public:
    ScannerBoxFor(Scanner &scanner)
      : ScannerBox(),
        scanner_(scanner)
    {}

    virtual void scanImpl(const void *addr, AllocThing *ptr) {
        scanner_(addr, ptr);
    }
};

class UpdaterBox
{
  protected:
    UpdaterBox() {}

  public:
    virtual AllocThing *updateImpl(void *addr, AllocThing *ptr) = 0;

    inline AllocThing *operator () (void *addr, AllocThing *ptr) {
        return updateImpl(addr, ptr);
    }
};

template <typename Updater>
class UpdaterBoxFor : public UpdaterBox
{
  private:
    Updater &updater_;

  public:
    UpdaterBoxFor(Updater &updater)
      : UpdaterBox(),
        updater_(updater)
    {}

    virtual AllocThing *updateImpl(void *addr, AllocThing *ptr) {
        return updater_(addr, ptr);
    }
};


void
ScanAllocThingImpl(ScannerBox &scanner, const AllocThing *thing,
               const void *start, const void *end);

void
UpdateAllocThingImpl(UpdaterBox &updater, AllocThing *thing,
                 const void *start, const void *end);


template <typename Scanner>
void
ScanAllocThing(Scanner &scanner, const AllocThing *thing,
               const void *start, const void *end)
{
    ScannerBoxFor<Scanner> scannerBox(scanner);
    ScanAllocThingImpl(scannerBox, thing, start, end);
}

template <typename Updater>
void
UpdateAllocThing(Updater &updater, AllocThing *thing,
                 const void *start, const void *end)
{
    UpdaterBoxFor<Updater> updaterBox(updater);
    UpdateAllocThingImpl(updaterBox, thing, start, end);
}


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__CORE_HPP

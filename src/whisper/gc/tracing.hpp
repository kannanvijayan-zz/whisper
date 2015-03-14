#ifndef WHISPER__GC__TRACING_HPP
#define WHISPER__GC__TRACING_HPP

#include <type_traits>

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"

#include "gc/formats.hpp"
#include "gc/stack_things.hpp"
#include "gc/heap_things.hpp"

//
//
// INTRODUCTION
// ============
//
// Traced structures interact with the GC in three different roles
//
//  1. As a stack-allocated structure.
//  2. As a heap-allocated structure.
//  3. As a field in a an allocated structure.
//
// A type can serve in more than one of these roles, including all of them.
//
// The interaction of the GC with types playing these different roles
// is different, and requires different information.
//
// TRACING
// =======
//
// To trace stack and heap allocated top-level structures, the GC
// first obtains the StackFormat or HeapFormat of the structure, and
// then maps this format to a type X using either StackFormatTraits or
// HeapFormatTraits.
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
//                  v                 v        |           |
//           +-------------+  +------------+   |           |
//           | StackFormat |  | HeapFormat |   |           |
//           +-------------+  +------------+   |           |
//               via |              | via      |           |
// StackFormatTraits |              | HeapFormatTraits     |
//                   +--------------+          |           |
//                           |                 |           |
//                           |                 |           |
//                           |                 |           |
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

namespace Whisper {


template <typename T>
constexpr inline bool IsTraceableThingType() {
    return IsHeapThingType<T>() || IsStackThingType<T>();
}
class TraceableThing
{
  private:
    TraceableThing() = delete;

  public:
    template <typename T>
    static TraceableThing *From(T *ptr) {
        static_assert(IsTraceableThingType<T>(),
                      "T is not TraceableThingType");
        return reinterpret_cast<TraceableThing *>(ptr);
    }

    template <typename T>
    static const TraceableThing *From(const T *ptr) {
        static_assert(IsTraceableThingType<T>(),
                      "T is not TraceableThingType");
        return reinterpret_cast<const TraceableThing *>(ptr);
    }
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
//          void scanner(const void *addr, HeapThing *ptr);
//
//      The |scanner| should be called for every reference to an HeapThing
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
//          HeapThing *updater(void *addr, HeapThing *ptr);
//
//      The |updater| should be called for every reference to an HeapThing
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

template <typename T>
struct UntracedTraceTraits
{
    UntracedTraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = true;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const T &t,
                     const void *start, const void *end)
    {}

    template <typename Updater>
    static void Update(Updater &updater, T &t,
                       const void *start, const void *end)
    {}
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
// By default, DerefTraits simply maps T to T*.
template <typename T>
struct DerefTraits
{
    DerefTraits() = delete;

    typedef T Type;
    typedef const T ConstType;

    // Given an reference to T, return a pointer to Type.
    static inline ConstType *Deref(const T &t) {
        return &t;
    }
    static inline Type *Deref(T &t) {
        return &t;
    }
};


///////////////////////////////////////////////////////////////////////////////

namespace GC {

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
    virtual void scanImpl(const void *addr, HeapThing *ptr) = 0;

    inline void operator () (const void *addr, HeapThing *ptr) {
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

    virtual void scanImpl(const void *addr, HeapThing *ptr) {
        scanner_(addr, ptr);
    }
};

class UpdaterBox
{
  protected:
    UpdaterBox() {}

  public:
    virtual HeapThing *updateImpl(void *addr, HeapThing *ptr) = 0;

    inline HeapThing *operator () (void *addr, HeapThing *ptr) {
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

    virtual HeapThing *updateImpl(void *addr, HeapThing *ptr) {
        return updater_(addr, ptr);
    }
};


void
ScanStackThingImpl(ScannerBox &scanner, const StackThing *thing,
                   const void *start, const void *end);

void
UpdateStackThingImpl(UpdaterBox &updater, StackThing *thing,
                     const void *start, const void *end);

template <typename Scanner>
void
ScanStackThing(Scanner &scanner, const StackThing *thing,
               const void *start, const void *end)
{
    ScannerBoxFor<Scanner> scannerBox(scanner);
    ScanStackThingImpl(scannerBox, thing, start, end);
}

template <typename Updater>
void
UpdateStackThing(Updater &updater, StackThing *thing,
                 const void *start, const void *end)
{
    UpdaterBoxFor<Updater> updaterBox(updater);
    UpdateStackThingImpl(updaterBox, thing, start, end);
}


void
ScanHeapThingImpl(ScannerBox &scanner, const HeapThing *thing,
                  const void *start, const void *end);

void
UpdateHeapThingImpl(UpdaterBox &updater, HeapThing *thing,
                    const void *start, const void *end);

template <typename Scanner>
void
ScanHeapThing(Scanner &scanner, const HeapThing *thing,
              const void *start, const void *end)
{
    ScannerBoxFor<Scanner> scannerBox(scanner);
    ScanHeapThingImpl(scannerBox, thing, start, end);
}

template <typename Updater>
void
UpdateHeapThing(Updater &updater, HeapThing *thing,
                const void *start, const void *end)
{
    UpdaterBoxFor<Updater> updaterBox(updater);
    UpdateHeapThingImpl(updaterBox, thing, start, end);
}


} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__TRACING_HPP

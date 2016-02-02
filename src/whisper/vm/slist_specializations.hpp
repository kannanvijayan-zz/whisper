#ifndef WHISPER__VM__SLIST_SPECIALIZATIONS_HPP
#define WHISPER__VM__SLIST_SPECIALIZATIONS_HPP


#include "vm/core.hpp"


// Specialize slists for primitive types.
#define DEF_PRIM_SLIST_TRAITS_(type, fmtName) \
  namespace Whisper { \
  namespace VM { \
    template <> \
    struct SlistTraits<type> { \
        SlistTraits() = delete; \
        static const bool Specialized = true; \
        static const HeapFormat SlistFormat = HeapFormat::fmtName; \
    }; \
  } \
  template <> \
  struct HeapFormatTraits<HeapFormat::fmtName> { \
      HeapFormatTraits() = delete; \
      static const bool Specialized = true; \
      typedef VM::Slist<type> Type; \
  }; \
  }

DEF_PRIM_SLIST_TRAITS_(uint8_t, UInt8Slist);
DEF_PRIM_SLIST_TRAITS_(uint16_t, UInt16Slist);
DEF_PRIM_SLIST_TRAITS_(uint32_t, UInt32Slist);
DEF_PRIM_SLIST_TRAITS_(uint64_t, UInt64Slist);
DEF_PRIM_SLIST_TRAITS_(int8_t, Int8Slist);
DEF_PRIM_SLIST_TRAITS_(int16_t, Int16Slist);
DEF_PRIM_SLIST_TRAITS_(int32_t, Int32Slist);
DEF_PRIM_SLIST_TRAITS_(int64_t, Int64Slist);
DEF_PRIM_SLIST_TRAITS_(float, FloatSlist);
DEF_PRIM_SLIST_TRAITS_(double, DoubleSlist);

#undef DEF_PRIM_SLIST_TRAITS_


namespace Whisper {
namespace VM {


// Specialize slists for general pointer types.
// Treat them by default as slists of pointers-to-alloc-things.
template <typename P>
struct SlistTraits<P*> {
    static_assert(IsHeapThingType<P>(), "P is not a HeapThing type.");
    SlistTraits() = delete;
    static constexpr bool Specialized = true;
    static const HeapFormat SlistFormat = HeapFormat::HeapPointerSlist;
};


} // namespace VM


//
// GC-Specializations for Slist
//


template <typename T>
struct HeapTraits<VM::Slist<T>>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = VM::SlistTraits<T>::SlistFormat;
    static constexpr bool VarSized = false;
};

template <>
struct HeapFormatTraits<HeapFormat::HeapPointerSlist>
{
    HeapFormatTraits() = delete;
    typedef VM::Slist<HeapThing*> Type;
};

template <typename T>
struct TraceTraits<VM::Slist<T>>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    typedef VM::Slist<T> Slist_;

    template <typename Scanner>
    static void Scan(Scanner& scanner, Slist_ const& slist,
                     void const* start, void const* end)
    {
        slist.value_.scan(scanner, start, end);
        slist.rest_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, Slist_& slist,
                       void const* start, void const* end)
    {
        slist.value_.update(updater, start, end);
        slist.rest_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SLIST_SPECIALIZATIONS_HPP

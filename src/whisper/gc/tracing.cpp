

#include "common.hpp"
#include "debug.hpp"
#include "gc/tracing.hpp"
#include "gc/specializations.hpp"

#include "parser/packed_syntax.hpp"
#include "parser/packed_writer.hpp"
#include "parser/packed_reader.hpp"

#include "vm/string.hpp"
#include "vm/properties.hpp"
#include "vm/array.hpp"
#include "vm/slist.hpp"
#include "vm/error.hpp"
#include "vm/source_file.hpp"
#include "vm/runtime_state.hpp"
#include "vm/exception.hpp"
#include "vm/box.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "vm/property_dict.hpp"
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"
#include "vm/scope_object.hpp"
#include "vm/global_scope.hpp"
#include "vm/lookup_state.hpp"
#include "vm/function.hpp"
#include "vm/frame.hpp"
#include "vm/self_traced.hpp"
#include "interp/heap_interpreter.hpp"

namespace Whisper {


char const*
StackFormatString(StackFormat fmt)
{
    switch (fmt) {
      case StackFormat::INVALID: return "INVALID";
#define CASE_(fmtName) case StackFormat::fmtName: return #fmtName;
    WHISPER_DEFN_GC_STACK_FORMATS(CASE_)
#undef CASE_
      case StackFormat::LIMIT: return "LIMIT";
      default: return "UNKNOWN";
    }
}

char const*
HeapFormatString(HeapFormat fmt)
{
    switch (fmt) {
      case HeapFormat::INVALID: return "INVALID";
#define CASE_(fmtName) case HeapFormat::fmtName: return #fmtName;
    WHISPER_DEFN_GC_HEAP_FORMATS(CASE_)
#undef CASE_
      case HeapFormat::LIMIT: return "LIMIT";
      default: return "UNKNOWN";
    }
}

char const*
GenString(Gen gen)
{
    switch (gen) {
      case Gen::None:        return "None";
      case Gen::Hatchery:    return "Hatchery";
      case Gen::Nursery:     return "Nursery";
      case Gen::Mature:      return "Mature";
      case Gen::Tenured:     return "Tenured";
      case Gen::Immortal:    return "Immortal";
      case Gen::LIMIT:       return "LIMIT";
    }
    return "UNKNOWN";
}


void
GC::ScanStackThingImpl(AbstractScanner& scanner, StackThing const* thing,
                       void const* start, void const* end)
{
    WH_ASSERT(thing != nullptr);
    switch (thing->format()) {
#define SWITCH_(fmtName) \
    case StackFormat::fmtName: {\
        typedef StackFormatTraits<StackFormat::fmtName>::Type T; \
        T const* tptr = reinterpret_cast<T const*>(thing); \
        TraceTraits<T>::Scan<AbstractScanner>(scanner, *tptr, start, end); \
        break; \
    }
      WHISPER_DEFN_GC_STACK_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD StackFormat");
        break;
    }
}

void
GC::UpdateStackThingImpl(AbstractUpdater& updater, StackThing* thing,
                         void const* start, void const* end)
{
    WH_ASSERT(thing != nullptr);
    switch (thing->format()) {
#define SWITCH_(fmtName) \
    case StackFormat::fmtName: {\
        typedef StackFormatTraits<StackFormat::fmtName>::Type T; \
        T* tptr = reinterpret_cast<T*>(thing); \
        TraceTraits<T>::Update<AbstractUpdater>(updater, *tptr, start, end); \
        break; \
    }
      WHISPER_DEFN_GC_STACK_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD StackFormat");
        break;
    }
}

void
GC::ScanHeapThingImpl(AbstractScanner& scanner, HeapThing const* thing,
                      void const* start, void const* end)
{
    WH_ASSERT(thing != nullptr);
    switch (thing->format()) {
#define SWITCH_(fmtName) \
    case HeapFormat::fmtName: {\
        typedef HeapFormatTraits<HeapFormat::fmtName>::Type T; \
        T const* tptr = reinterpret_cast<T const*>(thing); \
        TraceTraits<T>::Scan<AbstractScanner>(scanner, *tptr, start, end); \
        break; \
    }
      WHISPER_DEFN_GC_HEAP_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD HeapFormat");
        break;
    }
}

void
GC::UpdateHeapThingImpl(AbstractUpdater& updater, HeapThing* thing,
                        void const* start, void const* end)
{
    WH_ASSERT(thing != nullptr);
    switch (thing->format()) {
#define SWITCH_(fmtName) \
    case HeapFormat::fmtName: {\
        typedef HeapFormatTraits<HeapFormat::fmtName>::Type T; \
        T* tptr = reinterpret_cast<T*>(thing); \
        TraceTraits<T>::Update<AbstractUpdater>(updater, *tptr, start, end); \
        break; \
    }
      WHISPER_DEFN_GC_HEAP_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD HeapFormat");
        break;
    }
}


} // namespace Whisper

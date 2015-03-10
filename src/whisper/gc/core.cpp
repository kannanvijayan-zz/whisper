

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"
#include "gc/specializations.hpp"

#include "vm/string.hpp"
#include "vm/property_name.hpp"
#include "vm/array.hpp"
#include "vm/source_file.hpp"
#include "vm/box.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "parser/packed_writer.hpp"

namespace Whisper {
namespace GC {

const char *
AllocFormatString(AllocFormat fmt)
{
    switch (fmt) {
      case AllocFormat::INVALID: return "INVALID";
#define CASE_(T) case AllocFormat::T: return #T;
    WHISPER_DEFN_GC_ALLOC_FORMATS(CASE_)
#undef CASE_
      case AllocFormat::LIMIT: return "LIMIT";
      default: return "UNKNOWN";
    }
}

const char *
GenString(Gen gen)
{
    switch (gen) {
      case Gen::None:        return "None";
      case Gen::OnStack:     return "OnStack";
      case Gen::LocalHeap:   return "LocalHeap";
      case Gen::Hatchery:    return "Hatchery";
      case Gen::Nursery:     return "Nursery";
      case Gen::Mature:      return "Mature";
      case Gen::Tenured:     return "Tenured";
      case Gen::LIMIT:       return "LIMIT";
      default:               return "UNKNOWN";
    }
}

template <AllocFormat Format>
void
ScanAllocThingImpl_(ScannerBox &scanner, const AllocThing *thing,
                    const void *start, const void *end)
{
    typedef typename AllocFormatTraits<Format>::Type TraceType;
    if (!TraceTraits<TraceType>::IsLeaf) {
        const TraceType &tref = *reinterpret_cast<const TraceType *>(thing);
        TraceTraits<TraceType>::Scan(scanner, tref, start, end);
    }
}

template <AllocFormat Format>
void
UpdateAllocThingImpl_(UpdaterBox &updater, AllocThing *thing,
                      const void *start, const void *end)
{
    typedef typename AllocFormatTraits<Format>::Type TraceType;
    if (!TraceTraits<TraceType>::IsLeaf) {
        TraceType &tref = *reinterpret_cast<TraceType *>(thing);
        TraceTraits<TraceType>::Update(updater, tref, start, end);
    }
}


void
ScanAllocThingImpl(ScannerBox &scanner, const AllocThing *thing,
                   const void *start, const void *end)
{
    WH_ASSERT(thing != nullptr);
    switch (thing->format()) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        ScanAllocThingImpl_<AllocFormat::fmtName>(scanner, thing, \
                                                  start, end); \
        break; \
    }
      WHISPER_DEFN_GC_ALLOC_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD AllocFormat");
        break;
    }
}

void
UpdateAllocThingImpl(UpdaterBox &updater, AllocThing *thing,
                     const void *start, const void *end)
{
    WH_ASSERT(thing != nullptr);
    switch (thing->format()) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        UpdateAllocThingImpl_<AllocFormat::fmtName>(updater, thing, \
                                                    start, end); \
        break; \
    }
      WHISPER_DEFN_GC_ALLOC_FORMATS(SWITCH_)
#undef SWITCH_
      default:
        WH_ASSERT(!!!"BAD AllocFormat");
        break;
    }
}


} // namespace GC
} // namespace Whisper

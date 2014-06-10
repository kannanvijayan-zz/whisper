

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"
#include "gc/specializations.hpp"

#include "vm/array.hpp"
#include "vm/value_type.hpp"
#include "vm/source_file.hpp"
#include "vm/module.hpp"
#include "vm/lexical_namespace.hpp"

namespace Whisper {
namespace GC {

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
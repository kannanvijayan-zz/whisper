

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"
#include "gc/specializations.hpp"

#include "vm/array.hpp"

namespace GC {
namespace Whisper {


void
ScanAllocThingImpl(ScannerBox &scanner, const AllocThing *thing,
               const void *start, const void *end)
{
    WH_ASSERT(ptr != nullptr);
    switch (fmt) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        constexpr AllocFormat FMT = AllocFormat::fmtName; \
        typedef AllocFormatTraits<AllocFormat::fmtName>::Type TRACE_TYPE; \
        const TRACE_TYPE &tref = \
            *reinterpret_cast<const TRACE_TYPE *>(ptr); \
        TraceTraits<TRACE_TYPE>::Scan(scanner, tref, start, end); \
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
    WH_ASSERT(ptr != nullptr);
    switch (fmt) {
#define SWITCH_(fmtName) \
    case AllocFormat::fmtName: {\
        constexpr AllocFormat FMT = AllocFormat::fmtName; \
        typedef AllocFormatTraits<FMT>::Type TRACE_TYPE; \
        TRACE_TYPE &tref = \
            *reinterpret_cast<const TRACE_TYPE *>(ptr); \
        TraceTraits<TRACE_TYPE>::Update(updater, tref, start, end); \
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

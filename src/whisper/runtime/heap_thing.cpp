
#include "common.hpp"
#include "debug.hpp"

#include "runtime/heap_thing.hpp"

namespace Whisper {
namespace Runtime {


bool
IsValidHeapType(HeapType type)
{
    return (type > HeapType::INVALID) && (type < HeapType::LIMIT);
}

const char *
HeapTypeString(HeapType type)
{
    switch (type) {
      case HeapType::INVALID:           return "INVALID";
    #define CASE_(t) case HeapType::t:  return #t;
        WHISPER_DEFN_HEAP_TYPES(CASE_)
    #undef CASE_
      case HeapType::LIMIT:             return "LIMIT";
      default:
        break;
    }
    return "UNKNOWN";
}


} // namespace Runtime
} // namespace Whisper

#include "vm/core.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace VM {


void
Box::snprint(char *buf, size_t n) const
{
    if (isPointer()) {
        snprintf(buf, n, "ptr(%p)", pointer<HeapThing>());
        return;
    }
    if (isInteger()) {
        snprintf(buf, n, "int(%lld)", (long long) integer());
        return;
    }
    if (isUndefined()) {
        snprintf(buf, n, "undef");
        return;
    }
    if (isBoolean()) {
        snprintf(buf, n, "bool(%s)", boolean() ? "true" : "false");
        return;
    }
    if (isInvalid()) {
        snprintf(buf, n, "invalid");
        return;
    }
    WH_UNREACHABLE("Unknown box kind.");
}


} // namespace VM
} // namespace Whisper

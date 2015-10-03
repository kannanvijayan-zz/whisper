#include "vm/core.hpp"
#include "vm/box.hpp"
#include "vm/wobject.hpp"

namespace Whisper {
namespace VM {


void
Box::snprint(char* buf, size_t n) const
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

ValBox::ValBox(Box const& box)
  : Box(box)
{
    WH_ASSERT_IF(isPointer() && pointer<HeapThing>() != nullptr,
                 Wobject::IsWobject(pointer<HeapThing>()));
}

OkResult
ValBox::toString(ThreadContext* cx, std::ostringstream &out) const
{
    WH_ASSERT(isValid());
    if (isUndefined()) {
        out << "undefined";
        return OkVal();
    }

    if (isInteger()) {
        out << integer();
        return OkVal();
    }

    if (isBoolean()) {
        out << (boolean() ? "true" : "false");
        return OkVal();
    }

    if (isPointer()) {
        HeapThing *heapThing = HeapThing::From(objectPointer());
        out << "[Object " << heapThing->header().formatString() << "]";
        return OkVal();
    }

    WH_UNREACHABLE("Invalid valbox value.");
    return ErrorVal();
}


} // namespace VM
} // namespace Whisper

#include "vm/core.hpp"
#include "vm/box.hpp"
#include "vm/wobject.hpp"

namespace Whisper {
namespace VM {


size_t
Box::snprint(char* buf, size_t n) const
{
    if (isPointer()) {
        HeapThing* ht = pointer<HeapThing>();
        if (ht->isString()) {
            String* str = reinterpret_cast<String*>(ht);
            return snprintf(buf, n, "ptr(%s:%p:%s)", ht->formatString(), ht,
                                              str->c_chars());
        } else {
            return snprintf(buf, n, "ptr(%s:%p)", ht->formatString(), ht);
        }
    }
    if (isInteger())
        return snprintf(buf, n, "int(%lld)", (long long) integer());

    if (isUndefined())
        return snprintf(buf, n, "undef");

    if (isBoolean())
        return snprintf(buf, n, "bool(%s)", boolean() ? "true" : "false");

    if (isInvalid())
        return snprintf(buf, n, "invalid");

    WH_UNREACHABLE("Unknown box kind.");
    return std::numeric_limits<size_t>::max();
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

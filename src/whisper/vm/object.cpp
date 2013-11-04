
#include <algorithm>

#include "value_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/object.hpp"

namespace Whisper {
namespace VM {


HashObject::HashObject(Handle<Object *> prototype)
  : prototype_(prototype), entries_(0), mappings_(nullptr)
{}

bool
HashObject::initialize(RunContext *cx)
{
    WH_ASSERT(!mappings_);

    Tuple *tup;
    if (!cx->inHatchery().createTuple(INITIAL_ENTRIES * 2, tup))
        return false;

    mappings_.set(tup, this);
    if (!mappings_)
        return false;

    return true;
}

Handle<Object *>
HashObject::prototype() const
{
    return prototype_;
}

Handle<Tuple *>
HashObject::mappings() const
{
    return mappings_;
}


} // namespace VM
} // namespace Whisper

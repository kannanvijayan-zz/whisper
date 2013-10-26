
#include "vm/object.hpp"
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/shape_tree_inlines.hpp"
#include <algorithm>

namespace Whisper {
namespace VM {


HashObject::HashObject(Handle<Object *> prototype, Handle<Tuple *> mappings)
  : prototype_(prototype), mappings_(mappings)
{}

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

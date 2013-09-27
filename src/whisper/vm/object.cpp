
#include "vm/object.hpp"
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/shape_tree_inlines.hpp"
#include <algorithm>

namespace Whisper {
namespace VM {

ObjectImpl::ObjectImpl(Shape *shape)
  : PropertyMapThing(shape)
{
}

Object::Object(Shape *shape)
  : ObjectImpl(shape)
{
}


} // namespace VM
} // namespace Whisper


#include "vm/object.hpp"
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/shape_tree_inlines.hpp"
#include <algorithm>

namespace Whisper {
namespace VM {


Object::Object(Shape *shape, Object *prototype)
  : PropertyMapThing(shape, prototype)
{
}


} // namespace VM
} // namespace Whisper

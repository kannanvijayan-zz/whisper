
#include "vm/global.hpp"
#include "value_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/shape_tree_inlines.hpp"
#include <algorithm>

namespace Whisper {
namespace VM {


Global::Global(Shape *shape, Global *prototype)
  : PropertyMapThing(shape, prototype)
{
}


} // namespace VM
} // namespace Whisper

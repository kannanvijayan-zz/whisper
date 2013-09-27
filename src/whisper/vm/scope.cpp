
#include "vm/scope.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {


//
// ObjectScopeDescriptor
//

ObjectScopeDescriptor::ObjectScopeDescriptor(HeapThing *outerDescriptor)
  : HeapThing(), outerDescriptor_(outerDescriptor)
{}


//
// ObjectScope
//

ObjectScope::ObjectScope(ObjectScopeDescriptor *descriptor,
                         HeapThing *outerScope,
                         ShapedHeapThing *bindingObject)
  : HeapThing(), descriptor_(descriptor), outerScope_(outerScope),
    bindingObject_(bindingObject)
{}


//
// DeclarativeScopeDescriptor
//

DeclarativeScopeDescriptor::DeclarativeScopeDescriptor(
        HeapThing *outerDescriptor)
  : HeapThing(), outerDescriptor_(outerDescriptor)
{}

//
// DeclarativeScope
//

DeclarativeScope::DeclarativeScope(Shape *shape,
                                   DeclarativeScopeDescriptor *descriptor,
                                   HeapThing *outerScope)
  : PropertyMapThing(shape), descriptor_(descriptor), outerScope_(outerScope)
{}

//
// GlobalScope
//

GlobalScope::GlobalScope(Shape *shape, Global *globalObject)
  : PropertyMapThing(shape), globalObject_(globalObject)
{}


} // namespace VM
} // namespace Whisper

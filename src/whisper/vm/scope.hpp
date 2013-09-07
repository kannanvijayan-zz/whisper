#ifndef WHISPER__VM__SCOPE_HPP
#define WHISPER__VM__SCOPE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/shape_tree.hpp"
#include "vm/global.hpp"

namespace Whisper {
namespace VM {

//
// Scopes come in three major flavors:
//      ObjectScope - Expose a PropertyMap object's properties as a scope.
//                    Also used for global scopes.
//      DeclarativeScope - A PropertyMap defining scope bindings.
//      GlobalScope - Expose a Global object as a scope.
//
// ObjectScopeDescriptor
//      +-----------------------+
//      | outerDescriptor       |
//      +-----------------------+
//
// ObjectScope
//      +-----------------------+
//      | descriptor            |
//      | outerScope            |
//      | bindingObject         |
//      +-----------------------+
//
// DeclarativeScopeDescriptor
//      +-----------------------+
//      | outerDescriptor       |
//      | templateScope         |
//      +-----------------------+
//
// DeclarativeScope
//      +-----------------------+
//      | ...                   |
//      | descriptor            |
//      | outerScope            |
//      | ...                   |
//      +-----------------------+
//
// GlobalScope
//      +-----------------------+
//      | globalObject          |
//      +-----------------------+
//
//  outerDescriptor
//      The descripor for the outer scope.  Only valid for Object and
//      Declarative scopes.
//
//  descriptor
//      The corresponding descriptor object for the scope.
//
//  outerScope
//      The containing scope.
//
//  templateScope
//      A template scope object to copy from.
//
//  globalObject
//      The Global object defining the global scope |this|.
//
//

class ObjectScopeDescriptor
  : public HeapThing,
    public TypedHeapThing<HeapType::ObjectScopeDescriptor>
{
  private:
    NullableHeapThingValue<HeapThing> outerDescriptor_;
    
  public:
    ObjectScopeDescriptor(HeapThing *outerDescriptor);
};


class ObjectScope : public HeapThing,
                    public TypedHeapThing<HeapType::ObjectScope>
{
  private:
    HeapThingValue<ObjectScopeDescriptor> descriptor_;
    HeapThingValue<HeapThing> outerScope_;
    HeapThingValue<ShapedHeapThing> bindingObject_;

  public:
    ObjectScope(ObjectScopeDescriptor *descriptor, HeapThing *outerScope,
                ShapedHeapThing *bindingObject);
};

class DeclarativeScopeDescriptor
  : public HeapThing,
    public TypedHeapThing<HeapType::DeclarativeScopeDescriptor>
{
  private:
    NullableHeapThingValue<HeapThing> outerDescriptor_;
    
  public:
    DeclarativeScopeDescriptor(HeapThing *outerDescriptor);
};

template <>
struct PropertyMapTypeTraits<HeapType::DeclarativeScope>
{
    static constexpr uint32_t NumInternalSlots = 2;
};

class DeclarativeScope : public PropertyMapThing,
                         public TypedHeapThing<HeapType::DeclarativeScope>
{
  private:
    HeapThingValue<DeclarativeScopeDescriptor> descriptor_;
    HeapThingValue<HeapThing> outerScope_;

  public:
    DeclarativeScope(Shape *shape, PropertyMapThing *prototype,
                     DeclarativeScopeDescriptor *descriptor,
                     HeapThing *outerScope);
};

template <>
struct PropertyMapTypeTraits<HeapType::GlobalScope>
{
    static constexpr uint32_t NumInternalSlots = 1;
};

class GlobalScope : public PropertyMapThing,
                    public TypedHeapThing<HeapType::GlobalScope>
{
  private:
    HeapThingValue<Global> globalObject_;

  public:
    GlobalScope(Shape *shape, PropertyMapThing *prototype,
                Global *globalObject);
};

typedef HeapThingWrapper<ObjectScopeDescriptor> WrappedObjectScopeDescriptor;
typedef HeapThingWrapper<ObjectScope> WrappedObjectScope;
typedef HeapThingWrapper<DeclarativeScopeDescriptor>
        WrappedDeclarativeScopeDescriptor;
typedef HeapThingWrapper<DeclarativeScope> WrappedDeclarativeScope;
typedef HeapThingWrapper<GlobalScope> WrappedGlobalScope;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SCOPE_HPP

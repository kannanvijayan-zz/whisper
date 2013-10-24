#ifndef WHISPER__VM__PROPERTY_MAP_THING_HPP
#define WHISPER__VM__PROPERTY_MAP_THING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "vm/heap_thing.hpp"
#include "vm/shape_tree.hpp"
#include "vm/tuple.hpp"

namespace Whisper {
namespace VM {

//
// A PropertyMapThing is a base class that captures common behaviour between
// various common JS objects which act as maps of properties to values.
//
// There are two categories of PropertyMapThing, each of which are represented
// in a different way.
//
// The first category of PropertyMapThings are _shaped_, _native_ heap
// things.  These objects contain a shape, a pointer to a dynamically
// growable slot tuple, some type-specific data, and a fixed extra amount
// of space for "fixed data".  Most normal javascript objects fall into
// this category.  For these objects, the mapping of properties to their
// bindings is described by a shape.
// These include regular objects, function objects, heap-boxed primitives,
// and arrays.  Their general structure is:
//
//      +---------------+
//      | header        |
//      +---------------+
//      | shape         |
//      | dynamicSlots  |
//      | <OBJECT       |
//      |  SPECIFIC     |
//      |  INTERNAL     |
//      |  SLOTS>       |
//      | fixedSlot0    |
//      | ...           |
//      | fixedSlotN    |
//      +---------------+
//
//  Are descried by a shape tree.
//  Contain a dynamicSlots slot
//  Contain a number of fixed slots.
//  Bit 0 of the header word flags stores the "PreventExtensions" flag.
//
//
// The second category of PropertyMapThings contains _exotic_ objects,
// such as proxies, where the property mapping is defined by some opaque
// set of traps.  Their general structure is:
//
//      +---------------+
//      | header        |
//      +---------------+
//      | traps         |
//      | <OBJECT       |
//      |  SPECIFIC     |
//      |  INTERNAL     |
//      |  SLOTS>       |
//      +---------------+
//
//  trapInfo points to a PropertyTraps object, which hold function pointers
//  for all trap operations.
//
//

// Every specific object that inherits from PropertyMap must
// provide a specialization for PropertyMapTypeTraits that
// includes the documented fields.
template <HeapType HT>
struct PropertyMapTypeTraits
{
    /* static constexpr bool IsShaped
     *
     * Specifies whether this is a shaped object, or an exotic one.
     */

    /* static constexpr uint32_t BaseSize
     *
     * The size of the base data structure.  For exotic objects, this
     * is just the size of the object (which is equal to the size of the
     * struct).  For regular objets, this is the size of the structure
     * excluding any fixed slots.
     */
};

class ShapedPropertyMapThing : public ShapedHeapThing
{
  private:
    Heap<Tuple *> dynamicSlots_;

  public:
    ShapedPropertyMapThing(Shape *shape);

    bool isExtensible() const;
    void preventExtensions();

    bool hasDynamicSlots() const;
    Handle<Tuple *> maybeDynamicSlots() const;
    Handle<Tuple *> dynamicSlots() const;

    uint32_t numFixedSlots() const;
    uint32_t numDynamicSlots() const;
    uint32_t numSlots() const;

    Handle<Value> fixedSlotValue(uint32_t idx) const;
    Handle<Value> dynamicSlotValue(uint32_t idx) const;
    Handle<Value> slotValue(uint32_t idx) const;
};

class PropertyTraps : public HeapThing,
                      public TypedHeapThing<HeapType::PropertyTraps>
{
  public:
    PropertyTraps();
};

class TrappedPropertyMapThing : public HeapThing
{
  private:
    Heap<PropertyTraps *> traps_;

  public:
    TrappedPropertyMapThing(PropertyTraps *traps);

    Handle<PropertyTraps *> traps() const;
};

class PropertyMapThing : public HeapThing
{
  public:
    // Static function to calculate implicit slots.
    static bool IsShaped(HeapType ht);
    static uint32_t BaseSize(HeapType ht);

    static constexpr uint32_t PreventExtensionsFlag = 0x01;

  public:
    // Construct a shaped PropertyMapThing
    PropertyMapThing(Shape *shape);

    // Construct an exotic PropertyMapThing
    PropertyMapThing(PropertyTraps *traps);

    bool isShaped() const;
    bool isTrapped() const;

    uint32_t baseSize() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__PROPERTY_MAP_THING_HPP

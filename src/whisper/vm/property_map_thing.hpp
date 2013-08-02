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
// All PropertyMapThings have the following characteristics:
//  Contain a prototype slot.
//  Are descried by a shape tree.
//  Contain a dynamicSlots slot
//  Contain a number of fixed slots.
//  Bit 0 of the header word flags stores the "PreventExtensions" flag.
//
// These include regular objects, function objects, heap-boxed primitives,
// anda arrays.  Their general structure is:
//
//      +---------------+
//      | header        |
//      +---------------+
//      | shape         |
//      | prototype     |
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
// The header flags store the following info:
//      Bit 0       - PreventExtensions flag
//
// The HeapType tracks the type of object described.
//      Object
//      Function
//      NativeFunction
//      Array
//      Arguments
//      String
//      Number
//      Boolean
//      Date
//      etc..
//

// Every specific object that inherits from PropertyMap must
// provide a specialization for PropertyMapTypeTraits that
// includes the documented fields.
template <HeapType HT>
struct PropertyMapTypeTraits
{
    /* static constexpr uint32_t NumInternalSlots
     *
     * The number of internal slots defined by an object of that HeapType,
     * excluding those already defined by PropertyMapThing.
     */
};

class PropertyMapThing : public ShapedHeapThing
{
  public:
    // Implicit slots for PropertyMapThing is:
    //      shape
    //      prototype
    //      dynamicSlots
    static constexpr uint32_t BaseImplicitSlots = 3;

    // Static function to calculate implicit slots.
    static uint32_t NumInternalSlots(HeapType ht);

    enum Flags : uint8_t
    {
        PreventExtensions = 0x01
    };

  private:
    NullableHeapThingValue<PropertyMapThing> prototype_;
    NullableHeapThingValue<Tuple> dynamicSlots_;

  public:
    PropertyMapThing(Shape *shape, PropertyMapThing *prototype);

    bool isExtensible() const;
    void preventExtensions();

    uint32_t numImplicitSlots() const;

    PropertyMapThing *prototype() const;

    bool hasDynamicSlots() const;
    Tuple *maybeDynamicSlots() const;
    Tuple *dynamicSlots() const;

    uint32_t numFixedSlots() const;
    uint32_t numDynamicSlots() const;
    uint32_t numSlots() const;

    const Value &fixedSlotValue(uint32_t idx) const;
    const Value &dynamicSlotValue(uint32_t idx) const;
    const Value &slotValue(uint32_t idx) const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__PROPERTY_MAP_THING_HPP

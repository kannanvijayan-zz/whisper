#ifndef WHISPER__VM__SHAPE_TREE_HPP
#define WHISPER__VM__SHAPE_TREE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "vm/heap_thing.hpp"

#include <type_traits>

//
// Shape trees are used to describe the structure of objects which
// hold property definitions.  They are composed of a single
// ShapeTree object, along with a parent-and-sibling-linked tree of
// Shape objects.
//
//
//                      +---------------+
//                      |               |
//                      |   ShapeTree   |
//                      |               |
//                      +---------------+
//                             | 
//                             |firstRoot
//                             |
//              +--------------+
//              |
//              V
//      +---------------+                       +---------------+
//      |               |    nextSibling        |               |
//      |  Root Shape 0 |---------------------->|  Root Shape 1 |
//      |               |                       |               |
//      +---------------+                       +---------------+
//         ^    |                                   ^   |
//         |    |                                   |   |
//         |    |                    +--------------+---|-------------+
//         |    |                    |                  |             |
//         |    | firstChild         |    +-------------+             |
//   parent|    |                    |    |  firstChild               |
//         |    |              parent|    |                     parent|
//         |    V                    |    V                           |
//      +---------------+         +---------------+   next    +---------------+
//      |               |         |               |  Sibling  |               |
//      |  Child Shape  |         |  Child Shape  |---------->|  Child Shape  |
//      |               |         |               |           |               |
//      +---------------+         +---------------+           +---------------+
//
//
// ShapeTree objects may optionally have a 'parent' shape tree, pointing
// to another ShapeTree.  In these cases, the parent shape tree corresponds
// to the shape of the prototype of the object of the
//
// All objects whose shape is captured by a particular shape tree have
// the same number of fixed slots.  The ShapeTree object holds this
// number.
//
//                               +---------------+
//                               |               |
//                               |   ShapeTree   |
//                               |               |
//                               +---------------+
//                   children            |   ^
//           +---------------------------+   |
//           |                               |
//           |                +--------------+-----------+
//           V                |                          |
//    +---------------+       |  +---------------+       |  +---------------+
//    |               | next  |  |               | next  |  |               |
//    | ShapeTreeChild|--------->| ShapeTreeChild|--------->| ShapeTreeChild|
//    |               |       |  |               |       |  |               |
//    +---------------+       |  +---------------+       |  +---------------+
//           |                |     child|               |          |
//      child|           +----+          |         parent|     child|
//           V     parent|    |parent    V               |          V
//    +---------------+  |    |  +---------------+       |  +---------------+
//    |               |  |    |  |               |       |  |               |
//    |   ShapeTree   |--+    +--|   ShapeTree   |       +--|   ShapeTree   |
//    |               |          |               |          |               |
//    +---------------+          +---------------+          +---------------+
//
//

namespace Whisper {
namespace VM {

class Object;
class Class;

class ShapeTreeChild;

class Shape;
class ValueShape;
class ConstantShape;
class GetterShape;
class SetterShape;
class AccessorShape;

class ShapeTree : public HeapThing, public TypedHeapThing<HeapType::ShapeTree>
{
  friend class Shape;
  public:
    struct Config
    {
        uint32_t numFixedSlots;
        uint32_t version;
    };

  private:
    NullableHeapThingValue<ShapeTree> parentTree_;
    NullableHeapThingValue<Shape> firstRoot_;
    NullableHeapThingValue<ShapeTreeChild> childTrees_;

    // The info word below is a magic word storing information
    // about the structure of the object.
    // This info is organized as follows:
    //  Bits 0 to 7         - the number of fixed slots in the object.
    //  Bits 8 to 31        - the tree's version.  This gets incremented
    //                        every time the shape tree changes.
    Value info_;

    static constexpr unsigned NumFixedSlotsBits = 8;
    static constexpr unsigned NumFixedSlotsShift = 0;
    static constexpr uint64_t NumFixedSlotsMax =
        (UInt64(1) << NumFixedSlotsBits) - 1;
    static constexpr uint64_t NumFixedSlotsMask =
        UInt64(NumFixedSlotsMax) << NumFixedSlotsShift;

    static constexpr unsigned VersionBits = 24;
    static constexpr unsigned VersionShift = 8;
    static constexpr uint64_t VersionMax = (UInt64(1) << VersionBits) - 1;
    static constexpr uint64_t VersionMask = UInt64(VersionMax) << VersionShift;

    void initialize(const Config &config);

  public:
    ShapeTree(ShapeTree *parentTree, Shape *firstRoot, const Config &config);

    Shape *firstRoot();

    bool hasParentTree() const;

    ShapeTree *parentTree() const;

    uint32_t numFixedSlots() const;

    uint32_t version() const;
};

//
// A linked list of ShapeTreeChild instances links a parent shape tree
// to all of its children.
//
class ShapeTreeChild : public HeapThing,
                       public TypedHeapThing<HeapType::ShapeTreeChild>
{
  friend class Shape;
  friend class ShapeTree;
  private:
    NullableHeapThingValue<ShapeTreeChild> next_;
    HeapThingValue<ShapeTree> child_;

  public:
    ShapeTreeChild(ShapeTreeChild *next, ShapeTree *child);

    ShapeTreeChild *next() const;
    ShapeTree *child() const;
};

//
// Individual shapes are are linked-trees, formed from child
// and sibling pointers.
//
// A Shape object uses the header flag bits as follows:
//  Bit 0       - set if property has a value slot.
//  Bit 1       - set if property has a getter.
//  Bit 2       - set if property has a setter.
//  Bit 3       - set if property is configurable.
//  Bit 4       - set if property is enumerable.
//  Bit 5       - set if property is writable.
//
// Depending on the values of these bits, the size of a shape can
// vary.
//
//  A value shape has 1 extra magic field, |slotInfo|, holding the
//  index of the slot, and a flag indicating whether it's a dynamic
//  or fixed slot.
//
//  A constant (non-writable value) shape has 1 extra field holding the
//  slot value.
//
//  A getter shape has 1 extra heap thing field holding the getter.
//  A setter shape has 1 extra heap thing field holding the setter.
//  A getter+setter shape has 2 extra heap thing fields holding the accessors.
//
// The root shape for a shape tree is always an empty shape.
//
class Shape : public HeapThing, public TypedHeapThing<HeapType::Shape>
{
  friend class ShapeTree;
  protected:
    HeapThingValue<ShapeTree> tree_;
    NullableHeapThingValue<Shape> parent_;
    Value name_;
    NullableHeapThingValue<Shape> firstChild_;
    NullableHeapThingValue<Shape> nextSibling_;

    enum Flags : uint32_t
    {
        HasValue        = 0x01,
        HasGetter       = 0x02,
        HasSetter       = 0x04,
        IsConfigurable  = 0x08,
        IsEnumerable    = 0x10,
        IsWritable      = 0x20
    };

    Shape(ShapeTree *tree, Shape *parent, const Value &name,
          bool hasValue, bool hasGetter, bool hasSetter,
          bool isConfigurable, bool isEnumerable, bool isWritable);

  public:
    ShapeTree *tree() const;

    bool hasParent() const;
    Shape *maybeParent() const;
    Shape *parent() const;

    const Value &name() const;

    bool hasFirstChild() const;
    Shape *maybeFirstChild() const;
    Shape *firstChild() const;

    bool hasNextSibling() const;
    Shape *maybeNextSibling() const;
    Shape *nextSibling() const;

    void addChild(Shape *child);

    bool hasValue() const;
    bool hasGetter() const;
    bool hasSetter() const;
    bool isConfigurable() const;
    bool isEnumerable() const;
    bool isWritable() const;

    ValueShape *toValueShape();
    const ValueShape *toValueShape() const;

    ConstantShape *toConstantShape();
    const ConstantShape *toConstantShape() const;

    GetterShape *toGetterShape();
    const GetterShape *toGetterShape() const;

    SetterShape *toSetterShape();
    const SetterShape *toSetterShape() const;

    AccessorShape *toAccessorShape();
    const AccessorShape *toAccessorShape() const;

  protected:
    void setNextSibling(Shape *sibling);

    void setFirstChild(Shape *child);
};

class ValueShape : public Shape
{
  friend class ShapeTree;
  public:
    static constexpr unsigned SlotIndexShift = 0;
    static constexpr uint64_t SlotIndexMask = (UInt64(1) << 32) - 1u;

    static constexpr unsigned IsDynamicSlotShift = 32;
  private:
    Value slotInfo_;

  public:
    ValueShape(ShapeTree *tree, Shape *parent, const Value &name,
               uint32_t slotIndex, bool isDynamicSlot,
               bool isConfigurable, bool isEnumerable);

    uint32_t slotIndex() const;
    bool isDynamicSlot() const;
};

class ConstantShape : public Shape
{
  friend class ShapeTree;
  protected:
    Value constant_;

  public:
    ConstantShape(ShapeTree *tree, Shape *parent, const Value &name,
                  const Value &constant,
                  bool isConfigurable, bool isEnumerable);

    const Value &constant() const;
};

class GetterShape : public Shape
{
  friend class ShapeTree;
  protected:
    Value getter_;

  public:
    GetterShape(ShapeTree *tree, Shape *parent, const Value &name,
                const Value &getter,
                bool isConfigurable, bool isEnumerable);

    const Value &getter() const;
};

class SetterShape : public Shape
{
  friend class ShapeTree;
  protected:
    Value setter_;

  public:
    SetterShape(ShapeTree *tree, Shape *parent, const Value &name,
                const Value &setter,
                bool isConfigurable, bool isEnumerable);

    const Value &setter() const;
};

class AccessorShape : public Shape
{
  friend class ShapeTree;
  protected:
    Value getter_;
    Value setter_;

  public:
    AccessorShape(ShapeTree *tree, Shape *parent, const Value &name,
                  const Value &getter, const Value &setter,
                  bool isConfigurable, bool isEnumerable);

    const Value &getter() const;
    const Value &setter() const;
};


//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//
class ShapedHeapThing : public HeapThing
{
  protected:
    HeapThingValue<Shape> shape_;

    ShapedHeapThing(Shape *shape);

  public:
    Shape *shape() const;
    void setShape(Shape *shape);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

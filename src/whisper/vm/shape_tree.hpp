#ifndef WHISPER__VM__SHAPE_TREE_HPP
#define WHISPER__VM__SHAPE_TREE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
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
// to the shape of the prototype of the object being described.
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

    static constexpr uint32_t NumFixedSlotsMax = 0x7Fu;
    static constexpr uint32_t VersionMax = 0x1FFFFFFu;

  private:
    Heap<ShapeTree *> parentTree_;
    Heap<Shape *> firstRoot_;
    Heap<ShapeTreeChild *> childTrees_;

    uint32_t numFixedSlots_ : 7;
    uint32_t version_ : 25;

    void initialize(const Config &config);

  public:
    ShapeTree(ShapeTree *parentTree, Shape *firstRoot, const Config &config);

    bool hasParentTree() const;
    Handle<ShapeTree *> parentTree() const;

    Handle<Shape *> firstRoot() const;

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
    Heap<ShapeTreeChild *> next_;
    Heap<ShapeTree *> child_;

  public:
    ShapeTreeChild(ShapeTreeChild *next, ShapeTree *child);

    Handle<ShapeTreeChild *> next() const;
    Handle<ShapeTree *> child() const;
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
  public:
    enum Flags : uint32_t
    {
        HasValue        = 0x01,
        HasGetter       = 0x02,
        HasSetter       = 0x04,
        IsConfigurable  = 0x08,
        IsEnumerable    = 0x10,
        IsWritable      = 0x20
    };

    struct Config
    {
        bool hasValue;
        bool hasGetter;
        bool hasSetter;
        bool isConfigurable;
        bool isEnumerable;
        bool isWritable;

        Config();

        Config &setHasValue(bool hasValue);
        Config &setHasGetter(bool hasGetter);
        Config &setHasSetter(bool hasSetter);
        Config &setIsConfigurable(bool isConfigurable);
        Config &setIsEnumerable(bool isEnumerable);
        Config &setIsWritable(bool isWritable);
    };

  protected:
    Heap<ShapeTree *> tree_;
    Heap<Shape *> parent_;
    Heap<Value> name_;
    Heap<Shape *> firstChild_;
    Heap<Shape *> nextSibling_;

    Shape(ShapeTree *tree, Shape *parent, const Value &name,
          const Config &config);

  public:
    Handle<ShapeTree *> tree() const;

    bool hasParent() const;
    Handle<Shape *> maybeParent() const;
    Handle<Shape *> parent() const;

    Handle<Value> name() const;

    bool hasFirstChild() const;
    Handle<Shape *> maybeFirstChild() const;
    Handle<Shape *> firstChild() const;

    bool hasNextSibling() const;
    Handle<Shape *> maybeNextSibling() const;
    Handle<Shape *> nextSibling() const;

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
  private:
    uint32_t slotIndex_;

  public:
    ValueShape(ShapeTree *tree, Shape *parent, const Value &name,
               uint32_t slotIndex,
               bool isConfigurable, bool isEnumerable);

    uint32_t slotIndex() const;
    bool isDynamicSlot() const;
};

class ConstantShape : public Shape
{
  friend class ShapeTree;
  protected:
    Heap<Value> constant_;

  public:
    ConstantShape(ShapeTree *tree, Shape *parent, const Value &name,
                  const Value &constant,
                  bool isConfigurable, bool isEnumerable);

    Handle<Value> constant() const;
};

class GetterShape : public Shape
{
  friend class ShapeTree;
  protected:
    Heap<Value> getter_;

  public:
    GetterShape(ShapeTree *tree, Shape *parent, const Value &name,
                const Value &getter,
                bool isConfigurable, bool isEnumerable);

    Handle<Value> getter() const;
};

class SetterShape : public Shape
{
  friend class ShapeTree;
  protected:
    Heap<Value> setter_;

  public:
    SetterShape(ShapeTree *tree, Shape *parent, const Value &name,
                const Value &setter,
                bool isConfigurable, bool isEnumerable);

    Handle<Value> setter() const;
};

class AccessorShape : public Shape
{
  friend class ShapeTree;
  protected:
    Heap<Value> getter_;
    Heap<Value> setter_;

  public:
    AccessorShape(ShapeTree *tree, Shape *parent, const Value &name,
                  const Value &getter, const Value &setter,
                  bool isConfigurable, bool isEnumerable);

    Handle<Value> getter() const;
    Handle<Value> setter() const;
};


//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//
class ShapedHeapThing : public HeapThing
{
  protected:
    Heap<Shape *> shape_;

    ShapedHeapThing(Shape *shape);

  public:
    Handle<Shape *> shape() const;
    void setShape(Shape *shape);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

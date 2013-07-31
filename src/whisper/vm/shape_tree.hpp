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
//                              |
//                              |rootShape
//                              V
//                      +---------------+
//                      |               |
//                      |   Root Shape  |
//                      |               |
//                      +---------------+
//                         ^   |
//                         |   |
//          +--------------+---|-------------------------+
//          |                  |                         |
//          |   +--------------+                         |
//          |   |   firstChild                           |
//          |   |                                        |
//    parent|   |                                  parent|
//          |   V                                        |
//      +---------------+                       +---------------+
//      |               |    nextSibling        |               |
//      |  Child Shape  |---------------------->|  Child Shape  |
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
class Shape;
class Class;
class ShapeTreeChild;
class Shape;

class ShapeTree : public HeapThing<HeapType::ShapeTree>
{
  friend class Shape;
  public:
    struct Config
    {
        uint32_t numFixedSlots;
        uint32_t version;
    };

  private:
    HeapThingValue<ShapeTree> parentTree_;
    HeapThingValue<Shape> rootShape_;
    HeapThingValue<ShapeTreeChild> childTrees_;

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
    ShapeTree(ShapeTree *parentTree, Shape *rootShape, const Config &config);

    Shape *rootShape();

    bool hasParentTree() const;

    ShapeTree *parentTree() const;

    uint32_t numFixedSlots() const;

    uint32_t version() const;
};

//
// A linked list of ShapeTreeChild instances links a parent shape tree
// to all of its children.
//
class ShapeTreeChild : public HeapThing<HeapType::ShapeTreeChild>
{
  friend class Shape;
  friend class ShapeTree;
  private:
    HeapThingValue<ShapeTreeChild> next_;
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
class Shape : public HeapThing<HeapType::Shape>
{
  friend class ShapeTree;
  public:
    struct Config
    {
        unsigned slotNumber;
    };

  private:
    HeapThingValue<ShapeTree> tree_;
    HeapThingValue<Shape> parent_;
    Value name_;
    HeapThingValue<Shape> firstChild_;
    HeapThingValue<Shape> nextSibling_;

    // The info associated with the shape.  This is stored in a
    // magic value with the low 60 bits containing the data
    // below:
    //  Bits 0 to 32 - the slot number of the object.
    Value info_;

    static constexpr unsigned SlotNumberBits = 32;
    static constexpr unsigned SlotNumberShift = 0;
    static constexpr uint64_t SlotNumberMax =
        (UInt64(1) << SlotNumberBits) - 1;
    static constexpr uint64_t SlotNumberMask =
        UInt64(SlotNumberMax) << SlotNumberShift;

    void initialize(const Config &config);

  public:
    Shape(ShapeTree *tree, const Config &config);
    Shape(ShapeTree *tree, Shape *parent, Value name, const Config &config);

    ShapeTree *tree() const;
    Shape *parent() const;
    const Value &name() const;

    Shape *firstChild() const;
    Shape *nextSibling() const;

    void addChild(Shape *child);

  protected:
    void setNextSibling(Shape *sibling);

    void setFirstChild(Shape *child);
};


//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//
template <HeapType HT>
class ShapedHeapThing : public HeapThing<HT>
{
  protected:
    Value shape_;

    inline ShapedHeapThing(Shape *shape);

  public:
    inline Shape *shape() const;
    inline void setShape(Shape *shape);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

#ifndef WHISPER__VM__SHAPE_TREE_HPP
#define WHISPER__VM__SHAPE_TREE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "value_inlines.hpp"
#include "rooting.hpp"
#include "vm/heap_thing.hpp"
#include "vm/vm_helpers.hpp"

#include <type_traits>

namespace Whisper {
namespace VM {

class Object;
class Shape;
class Class;
class ShapeTreeChild;
class Shape;

//
// Shape trees are used to describe the structure of objects which
// hold property definitions.  They are composed of a single
// ShapeTree object, along with a parent-and-sibling-linked tree of
// Shape objects.
//
// ShapeTree objects may optionally have a 'parent' shape tree, which
// may point to another ShapeTree which describes a notional parent
// object for any object whose shape is captured by the child
// shape tree.
//
// All objects whose shape is captured by a particular shape tree have
// the same number of fixed slots.  The ShapeTree object holds this
// number.
//
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
    Value parentTree_;
    Value rootShape_;
    Value childTrees_;

    // The info word below is a magic word storing information
    // about the structure of the object.
    // This info is organized as follows:
    //  Bits 0 to 7         - the number of fixed slots in the object.
    //  Bits 8 to 31        - the tree's version.  This gets incremented
    //                        every time the tree's shape info changes.
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

    void initialize(const Config &config) {
        WH_ASSERT(config.numFixedSlots <= NumFixedSlotsMax);
        WH_ASSERT(config.version <= VersionMax);
        uint64_t value = 0;
        value |= UInt64(config.numFixedSlots) << NumFixedSlotsShift;
        value |= UInt64(config.version) << VersionShift;
        info_ = MagicValue(value);
    }
    
  public:
    template <typename T>
    inline ShapeTree(ShapeTree *parentTree, Shape *rootShape,
                     const Config &config);

    inline Shape *rootShape();

    bool hasParentTree() const {
        return !parentTree_.isNull();
    }

    ShapeTree *parentTree() const {
        WH_ASSERT(hasParentTree());
        return parentTree_.getNativeObject<ShapeTree>();
    }

    uint32_t numFixedSlots() const {
        return (info_.getMagicInt() & NumFixedSlotsMask) >> NumFixedSlotsShift;
    }

    uint32_t version() const {
        return (info_.getMagicInt() & VersionMask) >> VersionShift;
    }
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
    Value tree_;
    Value parent_;
    Value name_;
    Value firstChild_;
    Value nextSibling_;

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

    void initialize(const Config &config) {
        WH_ASSERT(config.slotNumber <= SlotNumberMax);
        uint64_t value = 0;
        value |= (config.slotNumber << SlotNumberShift);
        info_ = MagicValue(value);
    }
    
  public:
    Shape(ShapeTree *tree, const Config &config)
      : tree_(NativeObjectValue(tree)),
        parent_(NullValue()),
        name_(NullValue()),
        firstChild_(NullValue()),
        nextSibling_(NullValue())
    {
        initialize(config);
    }

    Shape(ShapeTree *tree, Shape *parent, Value name, const Config &config)
      : tree_(NativeObjectValue(tree)),
        parent_(NativeObjectValue<Shape>(parent)),
        name_(name),
        firstChild_(NullValue()),
        nextSibling_(NullValue())
    {
        WH_ASSERT(IsNormalizedPropertyId(name));
        initialize(config);
    }

    ShapeTree *tree() const {
        return tree_.getNativeObject<ShapeTree>();
    }

    Shape *parent() const {
        WH_ASSERT(parent_.isNull() || parent_.isNativeObject());
        return parent_.getNativeObject<Shape>();
    }

    const Value &name() const {
        return name_;
    }

    Shape *firstChild() const {
        WH_ASSERT(firstChild_.isNull() || firstChild_.isNativeObject());
        return firstChild_.getNativeObject<Shape>();
    }

    Shape *nextSibling() const {
        WH_ASSERT(nextSibling_.isNull() || nextSibling_.isNativeObject());
        return nextSibling_.getNativeObject<Shape>();
    }

    void addChild(Shape *child) {
        WH_ASSERT(child->nextSibling() == nullptr);
        WH_ASSERT(child->firstChild() == nullptr);
        WH_ASSERT(child->parent() == this);
        WH_ASSERT(!child->name().isNull());
        child->setNextSibling(firstChild());
        setFirstChild(child);
    }

  protected:
    void setNextSibling(Shape *sibling) {
        WH_ASSERT(sibling);
        noteWrite(&nextSibling_);
        nextSibling_ = NativeObjectValue(sibling);
    }

    void setFirstChild(Shape *child) {
        WH_ASSERT(child);
        noteWrite(&firstChild_);
        firstChild_ = NativeObjectValue(child);
    }
};


//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//
template <HeapType HT>
class ShapedHeapThing : public HeapThing<HT>
{
  protected:
    Value shape_;

    ShapedHeapThing(Shape *shape) {
        shape_ = NativeObjectValue(shape);
    }

  public:
    Shape *shape() const {
        return shape_.getNativeObject<Shape>();
    }

    void setShape(Shape *shape) {
        this->noteWrite(&shape_);
        shape_ = NativeObjectValue(shape);
    }
};

template <typename T>
inline
ShapeTree::ShapeTree(ShapeTree *parentTree, Shape *rootShape,
                     const Config &config)
  : parentTree_(parentTree ? NativeObjectValue(parentTree) : NullValue()),
    rootShape_(NativeObjectValue(rootShape)),
    childTrees_(UndefinedValue()),
    info_(UndefinedValue())
{
    static_assert(std::is_base_of<ShapedHeapThing<T::Type>, T>::value,
                  "ShapeTree delegate object must be itself shaped.");
    initialize(config);
}

inline Shape *
ShapeTree::rootShape() {
    return rootShape_.getNativeObject<Shape>();
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

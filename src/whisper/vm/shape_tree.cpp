
#include "value_inlines.hpp"
#include "vm/shape_tree.hpp"
#include "vm/shape_tree_inlines.hpp"
#include "vm/vm_helpers.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {

//
// ShapeTree
//

void
ShapeTree::initialize(const Config &config)
{
    WH_ASSERT(config.numFixedSlots <= NumFixedSlotsMax);
    WH_ASSERT(config.version <= VersionMax);
    uint64_t value = 0;
    value |= UInt64(config.numFixedSlots) << NumFixedSlotsShift;
    value |= UInt64(config.version) << VersionShift;
    info_ = MagicValue(value);
}

ShapeTree::ShapeTree(ShapeTree *parentTree, Shape *rootShape,
                     const Config &config)
  : parentTree_(parentTree ? NativeObjectValue(parentTree) : NullValue()),
    rootShape_(NativeObjectValue(rootShape)),
    childTrees_(UndefinedValue()),
    info_(UndefinedValue())
{
    initialize(config);
}

Shape *
ShapeTree::rootShape()
{
    return rootShape_.getNativeObject<Shape>();
}

bool
ShapeTree::hasParentTree() const
{
    return !parentTree_.isNull();
}

ShapeTree *
ShapeTree::parentTree() const
{
    WH_ASSERT(hasParentTree());
    return parentTree_.getNativeObject<ShapeTree>();
}

uint32_t
ShapeTree::numFixedSlots() const
{
    return (info_.getMagicInt() & NumFixedSlotsMask) >> NumFixedSlotsShift;
}

uint32_t
ShapeTree::version() const
{
    return (info_.getMagicInt() & VersionMask) >> VersionShift;
}

//
// Shape
//

void
Shape::initialize(const Config &config)
{
    WH_ASSERT(config.slotNumber <= SlotNumberMax);
    uint64_t value = 0;
    value |= (config.slotNumber << SlotNumberShift);
    info_ = MagicValue(value);
}
    
Shape::Shape(ShapeTree *tree, const Config &config)
  : tree_(NativeObjectValue(tree)),
    parent_(NullValue()),
    name_(NullValue()),
    firstChild_(NullValue()),
    nextSibling_(NullValue())
{
    initialize(config);
}

Shape::Shape(ShapeTree *tree, Shape *parent, Value name, const Config &config)
  : tree_(NativeObjectValue(tree)),
    parent_(NativeObjectValue<Shape>(parent)),
    name_(name),
    firstChild_(NullValue()),
    nextSibling_(NullValue())
{
    WH_ASSERT(IsNormalizedPropertyId(name));
    initialize(config);
}

ShapeTree *
Shape::tree() const
{
    return tree_.getNativeObject<ShapeTree>();
}

Shape *
Shape::parent() const
{
    WH_ASSERT(parent_.isNull() || parent_.isNativeObject());
    return parent_.getNativeObject<Shape>();
}

const Value &
Shape::name() const
{
    return name_;
}

Shape *
Shape::firstChild() const
{
    WH_ASSERT(firstChild_.isNull() || firstChild_.isNativeObject());
    return firstChild_.getNativeObject<Shape>();
}

Shape *
Shape::nextSibling() const
{
    WH_ASSERT(nextSibling_.isNull() || nextSibling_.isNativeObject());
    return nextSibling_.getNativeObject<Shape>();
}

void
Shape::addChild(Shape *child)
{
    WH_ASSERT(child->nextSibling() == nullptr);
    WH_ASSERT(child->firstChild() == nullptr);
    WH_ASSERT(child->parent() == this);
    WH_ASSERT(!child->name().isNull());
    child->setNextSibling(firstChild());
    setFirstChild(child);
}

void
Shape::setNextSibling(Shape *sibling)
{
    WH_ASSERT(sibling);
    noteWrite(&nextSibling_);
    nextSibling_ = NativeObjectValue(sibling);
}

void
Shape::setFirstChild(Shape *child)
{
    WH_ASSERT(child);
    noteWrite(&firstChild_);
    firstChild_ = NativeObjectValue(child);
}


} // namespace VM
} // namespace Whisper

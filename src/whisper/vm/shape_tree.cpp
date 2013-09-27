
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

ShapeTree::ShapeTree(ShapeTree *parentTree, Shape *firstRoot,
                     const Config &config)
  : parentTree_(parentTree),
    firstRoot_(firstRoot),
    childTrees_(),
    info_(UndefinedValue())
{
    initialize(config);
}

Shape *
ShapeTree::firstRoot()
{
    return firstRoot_.getNativeObject<Shape>();
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
// ShapeTreeChild
//

ShapeTreeChild::ShapeTreeChild(ShapeTreeChild *next, ShapeTree *child)
  : next_(next),
    child_(child)
{
    WH_ASSERT(child);
}

ShapeTreeChild *
ShapeTreeChild::next() const
{
    return next_;
}

ShapeTree *
ShapeTreeChild::child() const
{
    return child_;
}

//
// Shape
//

Shape::Shape(ShapeTree *tree, Shape *parent, const Value &name,
             bool hasValue, bool hasGetter, bool hasSetter,
             bool isConfigurable, bool isEnumerable, bool isWritable)
  : tree_(tree),
    parent_(parent),
    name_(name),
    firstChild_(),
    nextSibling_()
{
    WH_ASSERT(tree);
    WH_ASSERT(IsNormalizedPropertyId(name));
    WH_ASSERT_IF(hasValue, !hasGetter && !hasSetter);
    WH_ASSERT_IF(!hasValue, !isWritable);
    uint32_t flags = 0;
    if (hasValue)
        flags |= HasValue;
    if (hasGetter)
        flags |= HasGetter;
    if (hasSetter)
        flags |= HasSetter;
    if (isConfigurable)
        flags |= IsConfigurable;
    if (isEnumerable)
        flags |= IsEnumerable;
    if (isWritable)
        flags |= IsWritable;
    initFlags(flags);
}

ShapeTree *
Shape::tree() const
{
    return tree_;
}

bool
Shape::hasParent() const
{
    return parent_.hasHeapThing();
}

Shape *
Shape::maybeParent() const
{
    return parent_;
}

Shape *
Shape::parent() const
{
    WH_ASSERT(hasParent());
    return parent_;
}

const Value &
Shape::name() const
{
    return name_;
}

bool
Shape::hasFirstChild() const
{
    return firstChild_.hasHeapThing();
}

Shape *
Shape::maybeFirstChild() const
{
    return firstChild_;
}

Shape *
Shape::firstChild() const
{
    WH_ASSERT(hasFirstChild());
    return firstChild_;
}

bool
Shape::hasNextSibling() const
{
    return nextSibling_.hasHeapThing();
}

Shape *
Shape::maybeNextSibling() const
{
    return nextSibling_;
}

Shape *
Shape::nextSibling() const
{
    WH_ASSERT(hasNextSibling());
    return nextSibling_;
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

bool
Shape::hasValue() const
{
    return flags() & HasValue;
}

bool
Shape::hasGetter() const
{
    return flags() & HasGetter;
}

bool
Shape::hasSetter() const
{
    return flags() & HasSetter;
}

bool
Shape::isConfigurable() const
{
    return flags() & IsConfigurable;
}

bool
Shape::isEnumerable() const
{
    return flags() & IsEnumerable;
}

bool
Shape::isWritable() const
{
    return flags() & IsWritable;
}

void
Shape::setNextSibling(Shape *sibling)
{
    WH_ASSERT(sibling);
    noteWrite(&nextSibling_);
    nextSibling_ = sibling;
}

void
Shape::setFirstChild(Shape *child)
{
    WH_ASSERT(child);
    noteWrite(&firstChild_);
    firstChild_ = child;
}

ValueShape *
Shape::toValueShape()
{
    WH_ASSERT(hasValue() && isWritable());
    return reinterpret_cast<ValueShape *>(this);
}

const ValueShape *
Shape::toValueShape() const
{
    WH_ASSERT(hasValue() && isWritable());
    return reinterpret_cast<const ValueShape *>(this);
}

ConstantShape *
Shape::toConstantShape()
{
    WH_ASSERT(hasValue() && !isWritable());
    return reinterpret_cast<ConstantShape *>(this);
}

const ConstantShape *
Shape::toConstantShape() const
{
    WH_ASSERT(hasValue() && !isWritable());
    return reinterpret_cast<const ConstantShape *>(this);
}

GetterShape *
Shape::toGetterShape()
{
    WH_ASSERT(hasGetter() && !hasSetter());
    return reinterpret_cast<GetterShape *>(this);
}

const GetterShape *
Shape::toGetterShape() const
{
    WH_ASSERT(hasGetter() && !hasSetter());
    return reinterpret_cast<const GetterShape *>(this);
}

SetterShape *
Shape::toSetterShape()
{
    WH_ASSERT(hasSetter() && !hasGetter());
    return reinterpret_cast<SetterShape *>(this);
}

const SetterShape *
Shape::toSetterShape() const
{
    WH_ASSERT(hasSetter() && !hasGetter());
    return reinterpret_cast<const SetterShape *>(this);
}

AccessorShape *
Shape::toAccessorShape()
{
    WH_ASSERT(hasGetter() && hasSetter());
    return reinterpret_cast<AccessorShape *>(this);
}

const AccessorShape *
Shape::toAccessorShape() const
{
    WH_ASSERT(hasGetter() && hasSetter());
    return reinterpret_cast<const AccessorShape *>(this);
}

//
// ValueShape
//

ValueShape::ValueShape(ShapeTree *tree, Shape *parent, const Value &name,
                       uint32_t slotIndex, bool isDynamicSlot,
                       bool isConfigurable, bool isEnumerable)
  : Shape(tree, parent, name,
          /*hasValue=*/true, /*hasGetter=*/false, /*hasSetter=*/false,
          isConfigurable, isEnumerable, /*isWritable=*/true)
{
    uint64_t slotInfo = 0;
    slotInfo |= UInt64(slotIndex) << SlotIndexShift;
    if (isDynamicSlot)
        slotInfo |= UInt64(1) << IsDynamicSlotShift;
    slotInfo_ = MagicValue(slotInfo);
}

uint32_t
ValueShape::slotIndex() const
{
    return (slotInfo_.getMagicInt() >> SlotIndexShift) & SlotIndexMask;
}

bool
ValueShape::isDynamicSlot() const
{
    return (slotInfo_.getMagicInt() >> IsDynamicSlotShift) & 1u;
}

//
// ConstantShape
//

ConstantShape::ConstantShape(ShapeTree *tree, Shape *parent, const Value &name,
                             const Value &constant,
                             bool isConfigurable, bool isEnumerable)
  : Shape(tree, parent, name,
          /*hasValue=*/true, /*hasGetter=*/false, /*hasSetter=*/false,
          isConfigurable, isEnumerable, /*isWritable=*/false)
{
    constant_ = constant;
}

const Value &
ConstantShape::constant() const
{
    return constant_;
}

//
// GetterShape
//

GetterShape::GetterShape(ShapeTree *tree, Shape *parent, const Value &name,
                         const Value &getter,
                         bool isConfigurable, bool isEnumerable)
  : Shape(tree, parent, name,
          /*hasValue=*/false, /*hasGetter=*/true, /*hasSetter=*/false,
          isConfigurable, isEnumerable, /*isWritable=*/false)
{
    getter_ = getter;
}

const Value &
GetterShape::getter() const
{
    return getter_;
}

//
// SetterShape
//

SetterShape::SetterShape(ShapeTree *tree, Shape *parent, const Value &name,
                         const Value &setter,
                         bool isConfigurable, bool isEnumerable)
  : Shape(tree, parent, name,
          /*hasValue=*/false, /*hasGetter=*/false, /*hasSetter=*/true,
          isConfigurable, isEnumerable, /*isWritable=*/false)
{
    setter_ = setter;
}

const Value &
SetterShape::setter() const
{
    return setter_;
}

//
// AccessorShape
//

AccessorShape::AccessorShape(ShapeTree *tree, Shape *parent, const Value &name,
                             const Value &getter, const Value &setter,
                             bool isConfigurable, bool isEnumerable)
  : Shape(tree, parent, name,
          /*hasValue=*/false, /*hasGetter=*/true, /*hasSetter=*/true,
          isConfigurable, isEnumerable, /*isWritable=*/false)
{
    getter_ = getter;
    setter_ = setter;
}

const Value &
AccessorShape::getter() const
{
    return getter_;
}

const Value &
AccessorShape::setter() const
{
    return setter_;
}

//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//

ShapedHeapThing::ShapedHeapThing(Shape *shape)
  : shape_(shape)
{}

Shape *
ShapedHeapThing::shape() const
{
    return shape_;
}

void
ShapedHeapThing::setShape(Shape *shape)
{
    noteWrite(&shape_);
    shape_ = shape;
}


} // namespace VM
} // namespace Whisper

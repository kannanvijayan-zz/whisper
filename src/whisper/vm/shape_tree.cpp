
#include "value_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/shape_tree.hpp"
#include "vm/shape_tree_inlines.hpp"
#include "vm/vm_helpers.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {

//
// ShapeTree
//

ShapeTree::ShapeTree(ShapeTree *parentTree, Shape *firstRoot,
                     const Config &config)
  : parentTree_(parentTree),
    firstRoot_(firstRoot),
    childTrees_(nullptr),
    numFixedSlots_(config.numFixedSlots),
    version_(config.version)
{
    WH_ASSERT(config.numFixedSlots < NumFixedSlotsMax);
    WH_ASSERT(config.version < VersionMax);
}

bool
ShapeTree::hasParentTree() const
{
    return parentTree_;
}

Handle<ShapeTree *>
ShapeTree::parentTree() const
{
    WH_ASSERT(hasParentTree());
    return parentTree_;
}

Handle<Shape *>
ShapeTree::firstRoot() const
{
    return firstRoot_;
}

uint32_t
ShapeTree::numFixedSlots() const
{
    return numFixedSlots_;
}

uint32_t
ShapeTree::version() const
{
    return version_;
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

Handle<ShapeTreeChild *>
ShapeTreeChild::next() const
{
    return next_;
}

Handle<ShapeTree *>
ShapeTreeChild::child() const
{
    return child_;
}

//
// Shape
//

Shape::Config::Config()
  : hasValue(false),
    hasGetter(false),
    hasSetter(false),
    isConfigurable(false),
    isEnumerable(false),
    isWritable(false)
{}

Shape::Config &
Shape::Config::setHasValue(bool hasValue)
{
    this->hasValue = hasValue;
    return *this;
}

Shape::Config &
Shape::Config::setHasGetter(bool hasGetter)
{
    this->hasGetter = hasGetter;
    return *this;
}

Shape::Config &
Shape::Config::setHasSetter(bool hasSetter)
{
    this->hasSetter = hasSetter;
    return *this;
}

Shape::Config &
Shape::Config::setIsConfigurable(bool isConfigurable)
{
    this->isConfigurable = isConfigurable;
    return *this;
}

Shape::Config &
Shape::Config::setIsEnumerable(bool isEnumerable)
{
    this->isEnumerable = isEnumerable;
    return *this;
}

Shape::Config &
Shape::Config::setIsWritable(bool isWritable)
{
    this->isWritable = isWritable;
    return *this;
}


Shape::Shape(ShapeTree *tree, Shape *parent, const Value &name,
             const Config &config)
  : tree_(tree),
    parent_(parent),
    name_(name),
    firstChild_(nullptr),
    nextSibling_(nullptr)
{
    WH_ASSERT(tree);
    WH_ASSERT(IsNormalizedPropertyId(name));
    WH_ASSERT_IF(config.hasValue, !config.hasGetter && !config.hasSetter);
    WH_ASSERT_IF(!config.hasValue, !config.isWritable);
    uint32_t flags = 0;
    if (config.hasValue)
        flags |= HasValue;
    if (config.hasGetter)
        flags |= HasGetter;
    if (config.hasSetter)
        flags |= HasSetter;
    if (config.isConfigurable)
        flags |= IsConfigurable;
    if (config.isEnumerable)
        flags |= IsEnumerable;
    if (config.isWritable)
        flags |= IsWritable;
    initFlags(flags);
}

Handle<ShapeTree *>
Shape::tree() const
{
    return tree_;
}

bool
Shape::hasParent() const
{
    return parent_;
}

Handle<Shape *>
Shape::maybeParent() const
{
    return parent_;
}

Handle<Shape *>
Shape::parent() const
{
    WH_ASSERT(hasParent());
    return parent_;
}

Handle<Value>
Shape::name() const
{
    return name_;
}

bool
Shape::hasFirstChild() const
{
    return firstChild_;
}

Handle<Shape *>
Shape::maybeFirstChild() const
{
    return firstChild_;
}

Handle<Shape *>
Shape::firstChild() const
{
    WH_ASSERT(hasFirstChild());
    return firstChild_;
}

bool
Shape::hasNextSibling() const
{
    return nextSibling_;
}

Handle<Shape *>
Shape::maybeNextSibling() const
{
    return nextSibling_;
}

Handle<Shape *>
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
    WH_ASSERT(!child->name()->isNull());
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
    nextSibling_.set(sibling, this);
}

void
Shape::setFirstChild(Shape *child)
{
    WH_ASSERT(child);
    firstChild_.set(child, this);
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
                       uint32_t slotIndex, bool isConfigurable,
                       bool isEnumerable)
  : Shape(tree, parent, name,
          Shape::Config().setHasValue(true)
                         .setIsConfigurable(isConfigurable)
                         .setIsEnumerable(isEnumerable)
                         .setIsWritable(true)),
    slotIndex_(slotIndex)
{}

uint32_t
ValueShape::slotIndex() const
{
    return slotIndex_;
}

bool
ValueShape::isDynamicSlot() const
{
    return slotIndex_ < tree_->numFixedSlots();
}

//
// ConstantShape
//

ConstantShape::ConstantShape(ShapeTree *tree, Shape *parent, const Value &name,
                             const Value &constant,
                             bool isConfigurable, bool isEnumerable)
  : Shape(tree, parent, name,
          Shape::Config().setHasValue(true)
                         .setIsConfigurable(isConfigurable)
                         .setIsEnumerable(isEnumerable)
                         .setIsWritable(false)),
    constant_(constant)
{}

Handle<Value>
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
          Shape::Config().setHasGetter(true)
                         .setIsConfigurable(isConfigurable)
                         .setIsEnumerable(isEnumerable)),
    getter_(getter)
{}

Handle<Value>
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
          Shape::Config().setHasSetter(true)
                         .setIsConfigurable(isConfigurable)
                         .setIsEnumerable(isEnumerable)),
    setter_(setter)
{}

Handle<Value>
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
          Shape::Config().setHasGetter(true)
                         .setHasSetter(true)
                         .setIsConfigurable(isConfigurable)
                         .setIsEnumerable(isEnumerable)),
    getter_(getter),
    setter_(setter)
{}

Handle<Value>
AccessorShape::getter() const
{
    return getter_;
}

Handle<Value>
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

Handle<Shape *>
ShapedHeapThing::shape() const
{
    return shape_;
}

void
ShapedHeapThing::setShape(Shape *shape)
{
    shape_.set(shape, this);
}


} // namespace VM
} // namespace Whisper

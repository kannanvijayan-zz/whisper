#ifndef WHISPER__VM__SHAPE_TREE_HPP
#define WHISPER__VM__SHAPE_TREE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/vm_helpers.hpp"

namespace Whisper {
namespace VM {

class Shape;

//
// A shape tree is the foundation on which trees of shapes are
// built.
//
// Shape trees are used to describe the structure of objects which
// hold property definitions.
//
class ShapeTree : public HeapThing<HeapType::ShapeTree>
{
  private:
    Value root_;
    
  public:
    inline ShapeTree(Shape *root);
    inline Shape *root();
};

//
// Individual shapes are are linked-trees, formed from child
// and sibling pointers.
//
class Shape : public HeapThing<HeapType::Shape>
{
  private:
    Value tree_;
    Value parent_;
    Value name_;
    Value firstChild_;
    Value nextSibling_;
    
  public:
    Shape(ShapeTree *tree)
      : tree_(SpecialObjectValue(tree)),
        parent_(NullValue()),
        name_(NullValue()),
        firstChild_(NullValue()),
        nextSibling_(NullValue())
    {}

    Shape(ShapeTree *tree, Shape *parent, Value name)
      : tree_(SpecialObjectValue(tree)),
        parent_(SpecialObjectValue<Shape>(parent)),
        name_(name),
        firstChild_(NullValue()),
        nextSibling_(NullValue())
    {
        WH_ASSERT(IsNormalizedPropertyId(name));
    }

    ShapeTree *tree() const {
        return tree_.getSpecialObject<ShapeTree>();
    }

    Shape *parent() const {
        WH_ASSERT(parent_.isNull() || parent_.isSpecialObject());
        return parent_.getSpecialObject<Shape>();
    }

    const Value &name() const {
        return name_;
    }

    Shape *firstChild() const {
        WH_ASSERT(firstChild_.isNull() || firstChild_.isSpecialObject());
        return firstChild_.getSpecialObject<Shape>();
    }

    Shape *nextSibling() const {
        WH_ASSERT(nextSibling_.isNull() || nextSibling_.isSpecialObject());
        return nextSibling_.getSpecialObject<Shape>();
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
        nextSibling_ = SpecialObjectValue(sibling);
    }

    void setFirstChild(Shape *child) {
        WH_ASSERT(child);
        noteWrite(&firstChild_);
        firstChild_ = SpecialObjectValue(child);
    }
};

inline
ShapeTree::ShapeTree(Shape *root)
  : root_(SpecialObjectValue(root))
{}

inline Shape *
ShapeTree::root() {
    return root_.getSpecialObject<Shape>();
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

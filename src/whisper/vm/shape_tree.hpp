#ifndef WHISPER__VM__SHAPE_TREE_HPP
#define WHISPER__VM__SHAPE_TREE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/heap_thing.hpp"
#include "vm/vm_helpers.hpp"

namespace Whisper {
namespace VM {

class Object;
class Shape;
class Class;

//
// A shape tree is the foundation on which trees of shapes are
// built.  Shape trees are parameterized on a two-tuple (klass, delegate),
// where klass is an object Class, and delegate is an inherited ancestor
// object.
//
// Most commonly, the delegate is either an object which is the prototype
// of all objects described by the shape tree, or an object which is an
// outer lexical environment to all lexical environments described by
// the shape tree.
//
// Shape trees are used to describe the structure of objects which
// hold property definitions.
//
class ShapeTree : public HeapThing<HeapType::ShapeTree>
{
  private:
    Value klass_;
    Value delegate_;
    Value root_;
    
  public:
    inline ShapeTree(Class &klass, Object *delegate, Shape *root);

    template <typename T>
    inline ShapeTree(Class &klass, T *delegate, Shape *root);

    inline Shape *root();

    bool hasDelegate() const {
        return !delegate_.isNull();
    }

    bool hasNativeObjectDelegate() const {
        return delegate_.isNativeObject();
    }

    bool hasSpecialObjectDelegate() const {
        return delegate_.isSpecialObject();
    }

    Object *nativeObjectDelegate() const {
        WH_ASSERT(hasNativeObjectDelegate());
        return delegate_.getNativeObject();
    }

    Class &klass() const {
        return *klass_.getForeignObject<Class>();
    }

    template <typename T>
    T *specialObjectDelegate() const {
        WH_ASSERT(hasSpecialObjectDelegate());
        return delegate_.getSpecialObject<T>();
    }
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
ShapeTree::ShapeTree(Class &klass, Object *delegate, Shape *root)
  : klass_(ForeignObjectValue(&klass)),
    delegate_(delegate ? ObjectValue(delegate) : NullValue()),
    root_(SpecialObjectValue(root))
{}

template <typename T>
inline
ShapeTree::ShapeTree(Class &klass, T *delegate, Shape *root)
  : klass_(ForeignObjectValue(&klass)),
    delegate_(delegate ? SpecialObjectValue(delegate) : NullValue()),
    root_(SpecialObjectValue(root))
{}

inline Shape *
ShapeTree::root() {
    return root_.getSpecialObject<Shape>();
}


//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//
template <HeapType HT>
class ShapedHeapThing : public HeapThing<HT>
{
  protected:
    Value shape_;

    ShapedHeapThing(Shape *shape) {
        shape_ = SpecialObjectValue(shape);
    }

  public:
    Shape *shape() const {
        return shape_.getSpecialObject<Shape>();
    }

    void setShape(Shape *shape) {
        this->noteWrite(&shape_);
        shape_ = SpecialObjectValue(shape);
    }
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

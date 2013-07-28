#ifndef WHISPER__VM__SHAPE_TREE_HPP
#define WHISPER__VM__SHAPE_TREE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/heap_thing.hpp"
#include "vm/vm_helpers.hpp"

#include <type_traits>

namespace Whisper {
namespace VM {

class Object;
class Shape;
class Class;

template <HeapType HT> class ShapedHeapThing;

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
    // Foreign pointer referring to Class
    Value klass_;
    Value delegate_;
    Value root_;
    
  public:
    template <typename T>
    inline ShapeTree(Class &klass, T *delegate, Shape *root);

    inline Shape *root();

    bool hasDelegate() const {
        return !delegate_.isNull();
    }

    template <typename T=Object>
    T *delegate() const {
        WH_ASSERT(hasDelegate());
        return delegate_.getNativeObject<T>();
    }

    Class &klass() const {
        return *klass_.getForeignObject<Class>();
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
      : tree_(NativeObjectValue(tree)),
        parent_(NullValue()),
        name_(NullValue()),
        firstChild_(NullValue()),
        nextSibling_(NullValue())
    {}

    Shape(ShapeTree *tree, Shape *parent, Value name)
      : tree_(NativeObjectValue(tree)),
        parent_(NativeObjectValue<Shape>(parent)),
        name_(name),
        firstChild_(NullValue()),
        nextSibling_(NullValue())
    {
        WH_ASSERT(IsNormalizedPropertyId(name));
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
ShapeTree::ShapeTree(Class &klass, T *delegate, Shape *root)
  : klass_(ForeignObjectValue(&klass)),
    delegate_(delegate ? NativeObjectValue(delegate) : NullValue()),
    root_(NativeObjectValue(root))
{
    static_assert(std::is_base_of<ShapedHeapThing<T::Type>, T>::value,
                  "ShapeTree delegate object must be itself shaped.");
}

inline Shape *
ShapeTree::root() {
    return root_.getNativeObject<Shape>();
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_HPP

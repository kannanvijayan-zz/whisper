#ifndef WHISPER__VM__SHAPE_TREE_INLINES_HPP
#define WHISPER__VM__SHAPE_TREE_INLINES_HPP

#include "vm/shape_tree.hpp"

namespace Whisper {
namespace VM {

//
// A ShapedHeapThing is a HeapThing whose structure is described by a Shape.
//

template <HeapType HT>
inline
ShapedHeapThing<HT>::ShapedHeapThing(Shape *shape)
  : shape_(shape)
{}

template <HeapType HT>
inline Shape *
ShapedHeapThing<HT>::shape() const
{
    return shape_.getHeapThing();
}

template <HeapType HT>
inline void
ShapedHeapThing<HT>::setShape(Shape *shape)
{
    this->noteWrite(&shape_);
    shape_ = shape;
}


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SHAPE_TREE_INLINES_HPP

#include "rooting.hpp"
#include "rooting_inlines.hpp"

namespace Whisper {

//
// RootBase
//

RootBase::RootBase(ThreadContext *threadContext, RootKind kind)
  : threadContext_(threadContext),
    next_(threadContext->roots()),
    kind_(kind)
{}

void
RootBase::postInit()
{
    threadContext_->roots_ = this;
}

ThreadContext *
RootBase::threadContext() const
{
    return threadContext_;
}

RootBase *
RootBase::next() const
{
    return next_;
}

RootKind
RootBase::kind() const
{
    return kind_;
}

//
// Root<Value>
//

Root<Value>::Root(RunContext *cx)
  : TypedRootBase<Value>(cx, RootKind::Value, Value::Undefined())
{}

Root<Value>::Root(ThreadContext *cx)
  : TypedRootBase<Value>(cx, RootKind::Value, Value::Undefined())
{}

Root<Value>::Root(RunContext *cx, const Value &val)
  : TypedRootBase<Value>(cx, RootKind::Value, val)
{}

Root<Value>::Root(ThreadContext *cx, const Value &val)
  : TypedRootBase<Value>(cx, RootKind::Value, val)
{}

const Value *
Root<Value>::operator ->() const
{
    return &thing_;
}

Value *
Root<Value>::operator ->()
{
    return &thing_;
}

Root<Value> &
Root<Value>::operator =(const Value &other)
{
    this->TypedRootBase<Value>::operator =(other);
    return *this;
}


//
// Heap<Value>
//

Heap<Value>::Heap()
  : TypedHeapBase<Value>(Value::Undefined())
{}

Heap<Value>::Heap(const Value &val)
  : TypedHeapBase<Value>(val)
{}

const Value *
Heap<Value>::operator ->() const
{
    return &val_;
}

Value *
Heap<Value>::operator ->()
{
    return &val_;
}


//
// Handle<Value>
//

Handle<Value>::Handle(const Value &locn)
  : TypedHandleBase<Value>(locn)
{}

Handle<Value>::Handle(const Root<Value> &root)
  : TypedHandleBase<Value>(root.get())
{}

Handle<Value>::Handle(const Heap<Value> &heap)
  : TypedHandleBase<Value>(heap.get())
{}

Handle<Value>::Handle(const MutHandle<Value> &mut)
  : TypedHandleBase<Value>(mut.get())
{}

/*static*/ Handle<Value>
Handle<Value>::FromTracedLocation(const Value &locn)
{
    return Handle<Value>(locn);
}

const Value *
Handle<Value>::operator ->()
{
    return &ref_;
}


//
// MutHandle<Value>
//

MutHandle<Value>::MutHandle(Value *val)
  : TypedMutHandleBase<Value>(val)
{}

MutHandle<Value>::MutHandle(Root<Value > *root)
  : TypedMutHandleBase<Value>(root->addr())
{}

MutHandle<Value>::MutHandle(Heap<Value> *heap)
  : TypedMutHandleBase<Value>(heap->addr())
{}

/*static*/ MutHandle<Value>
MutHandle<Value>::FromTracedLocation(Value *locn)
{
    return MutHandle<Value>(locn);
}

const Value *
MutHandle<Value>::operator ->() const
{
    return &ref_;
}

Value *
MutHandle<Value>::operator ->()
{
    return &ref_;
}

MutHandle<Value> &
MutHandle<Value>::operator =(const Value &other)
{
    ref_ = other;
    return *this;
}

//
// VectorRoot<Value>
//

VectorRoot<Value>::VectorRoot(RunContext *cx)
  : VectorRootBase<Value>(cx, RootKind::ValueVector)
{}

VectorRoot<Value>::VectorRoot(ThreadContext *cx)
  : VectorRootBase<Value>(cx, RootKind::ValueVector)
{}


} // namespace Whisper

#include "value_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/stack_frame.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/script.hpp"

#include <algorithm>

namespace Whisper {
namespace VM {

//
// StackFrame
//

/*static*/ uint32_t
StackFrame::CalculateSize(const Config &config)
{
    WH_ASSERT(config.numArgs >= config.numPassedArgs);

    uint32_t size = AlignIntUp<uint32_t>(sizeof(StackFrame), sizeof(Value));
    size += config.numPassedArgs * sizeof(Value);
    size += config.numLocals * sizeof(Value);
    size += config.maxStackDepth * sizeof(Value);
    return size;
}

StackFrame::StackFrame(Script *script, const Config &config)
  : callerFrame_(nullptr),
    callee_(script),
    pcOffset_(0),
    numPassedArgs_(config.numPassedArgs),
    numArgs_(config.numArgs),
    numLocals_(config.numLocals),
    stackDepth_(0)
{
    WH_ASSERT(config.numPassedArgs == numPassedArgs_);
}

bool
StackFrame::hasCallerFrame() const
{
    return callerFrame_;
}

Handle<StackFrame *>
StackFrame::callerFrame() const
{
    WH_ASSERT(hasCallerFrame());
    return callerFrame_;
}

bool
StackFrame::isScriptFrame() const
{
    return callee_->isScript();
}

Handle<Script *>
StackFrame::script() const
{
    WH_ASSERT(callee_->isScript());
    return reinterpret_cast<const Heap<Script *> &>(callee_);
}

bool
StackFrame::isTopLevelFrame() const
{
    return isScriptFrame() && script()->isTopLevel();
}

uint32_t
StackFrame::pcOffset() const
{
    return pcOffset_;
}

void 
StackFrame::setPcOffset(uint32_t newPcOffset)
{
    pcOffset_ = newPcOffset;
}

uint32_t
StackFrame::numPassedArgs() const
{
    return numPassedArgs_;
}

uint32_t
StackFrame::numArgs() const
{
    return numArgs_;
}

uint32_t
StackFrame::numLocals() const
{
    return numLocals_;
}

uint32_t
StackFrame::stackDepth() const
{
    return stackDepth_;
}

uint32_t
StackFrame::maxStackDepth() const
{
    return script()->maxStackDepth();
}

Handle<Value>
StackFrame::getArg(uint32_t idx) const
{
    WH_ASSERT(idx < numArgs());
    return argRef(idx);
}

Handle<Value>
StackFrame::getLocal(uint32_t idx) const
{
    WH_ASSERT(idx < numLocals());
    return localRef(idx);
}

void
StackFrame::pushStack(const Value &val)
{
    WH_ASSERT(stackDepth_ < maxStackDepth());
    stackRef(stackDepth_).set(val, this);
    stackDepth_++;
}

Handle<Value>
StackFrame::peekStack(int32_t offset) const
{
    WH_ASSERT(offset < 0);
    WH_ASSERT(ToUInt32(-offset) <= stackDepth_);
    return stackRef(stackDepth_ + offset);
}

Handle<Value>
StackFrame::getStack(uint32_t offset) const
{
    WH_ASSERT(offset < stackDepth_);
    return stackRef(offset);
}

void
StackFrame::popStack(uint32_t count)
{
    WH_ASSERT(stackDepth_ > 0);
    WH_ASSERT(count <= stackDepth_);

    if (count == 0)
        return;

    Value *start = &stackAt(stackDepth_ - count);
    Value *end = start + count;
    std::fill(start, end, Value::Undefined());
    stackDepth_ -= count;
}

const Value *
StackFrame::argStart() const
{
    const char *thisp = reinterpret_cast<const char *>(this);
    uint32_t adj = AlignIntUp<uint32_t>(sizeof(StackFrame), sizeof(Value));
    return reinterpret_cast<const Value *>(thisp + adj);
}

Value *
StackFrame::argStart()
{
    char *thisp = reinterpret_cast<char *>(this);
    uint32_t adj = AlignIntUp<uint32_t>(sizeof(StackFrame), sizeof(Value));
    return reinterpret_cast<Value *>(thisp + adj);
}

const Value *
StackFrame::localStart() const
{
    return argStart() + numArgs();
}

Value *
StackFrame::localStart()
{
    return argStart() + numArgs();
}

const Value *
StackFrame::stackStart() const
{
    return localStart() + numLocals();
}

Value *
StackFrame::stackStart()
{
    return localStart() + numLocals();
}

const Value &
StackFrame::argAt(uint32_t idx) const
{
    WH_ASSERT(idx < numArgs());
    return argStart()[idx];
}

Value &
StackFrame::argAt(uint32_t idx)
{
    WH_ASSERT(idx < numArgs());
    return argStart()[idx];
}

const Value &
StackFrame::localAt(uint32_t idx) const
{
    WH_ASSERT(idx < numLocals());
    return localStart()[idx];
}

Value &
StackFrame::localAt(uint32_t idx)
{
    WH_ASSERT(idx < numLocals());
    return localStart()[idx];
}

const Value &
StackFrame::stackAt(uint32_t idx) const
{
    WH_ASSERT(idx < maxStackDepth());
    return stackStart()[idx];
}

Value &
StackFrame::stackAt(uint32_t idx)
{
    WH_ASSERT(idx < maxStackDepth());
    return stackStart()[idx];
}

const Heap<Value> &
StackFrame::argRef(uint32_t idx) const
{
    return reinterpret_cast<const Heap<Value> &>(argAt(idx));
}

Heap<Value> &
StackFrame::argRef(uint32_t idx)
{
    return reinterpret_cast<Heap<Value> &>(argAt(idx));
}

const Heap<Value> &
StackFrame::localRef(uint32_t idx) const
{
    return reinterpret_cast<const Heap<Value> &>(localAt(idx));
}

Heap<Value> &
StackFrame::localRef(uint32_t idx)
{
    return reinterpret_cast<Heap<Value> &>(localAt(idx));
}

const Heap<Value> &
StackFrame::stackRef(uint32_t idx) const
{
    return reinterpret_cast<const Heap<Value> &>(stackAt(idx));
}

Heap<Value> &
StackFrame::stackRef(uint32_t idx)
{
    return reinterpret_cast<Heap<Value> &>(stackAt(idx));
}


} // namespace VM
} // namespace Whisper

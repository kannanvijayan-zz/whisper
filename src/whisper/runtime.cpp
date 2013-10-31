
#include <string.h>

#include "slab.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/stack_frame.hpp"
#include "vm/string.hpp"
#include "vm/double.hpp"
#include "vm/tuple.hpp"

namespace Whisper {

//
// Runtime
//

Runtime::Runtime()
  : threadContexts_()
{}

Runtime::~Runtime()
{}

bool
Runtime::initialize()
{
    WH_ASSERT(!initialized_);

    int error = pthread_key_create(&threadKey_, nullptr);
    if (error) {
        error_ = strerror_r(error, errorBuffer_, ErrorBufferSize);
        errorBuffer_[ErrorBufferSize-1] = '\0';
        return false;
    }

    initialized_ = true;
    return true;
}

bool
Runtime::hasError() const
{
    return error_ != nullptr;
}

const char *
Runtime::error() const
{
    WH_ASSERT(hasError());
    return error_;
}

const char *
Runtime::registerThread()
{
    WH_ASSERT(initialized_);
    WH_ASSERT(pthread_getspecific(threadKey_) == nullptr);

    // Create a new nursery slab.
    Slab *hatchery = Slab::AllocateStandard(Slab::Hatchery);
    if (!hatchery)
        return "Could not allocate hatchery slab.";

    // Create initial tenured space slab.
    Slab *tenured = Slab::AllocateStandard(Slab::Tenured);
    if (!tenured)
        return "Could not allocate tenured slab.";

    // Allocate the ThreadContext
    ThreadContext *ctx;
    try {
        ctx = new ThreadContext(this, hatchery, tenured);
        threadContexts_.push_back(ctx);
    } catch (std::bad_alloc &err) {
        return "Could not allocate ThreadContext.";
    }

    // Associate the thread context with the thread.
    int error = pthread_setspecific(threadKey_, ctx);
    if (error) {
        delete ctx;
        Slab::Destroy(hatchery);
        return "pthread_setspecific failed to set ThreadContext.";
    }

    return nullptr;
}

ThreadContext *
Runtime::maybeThreadContext()
{
    WH_ASSERT(initialized_);

    return reinterpret_cast<ThreadContext *>(pthread_getspecific(threadKey_));
}

bool
Runtime::hasThreadContext()
{
    WH_ASSERT(initialized_);

    return maybeThreadContext() != nullptr;
}

ThreadContext *
Runtime::threadContext()
{
    WH_ASSERT(initialized_);

    ThreadContext *ctx = maybeThreadContext();
    WH_ASSERT(ctx);
    return ctx;
}


//
// ThreadContext
//

ThreadContext::ThreadContext(Runtime *runtime, Slab *hatchery, Slab *tenured)
  : runtime_(runtime),
    hatchery_(hatchery),
    nursery_(nullptr),
    tenured_(tenured),
    tenuredList_(),
    activeRunContext_(nullptr),
    runContextList_(nullptr),
    roots_(nullptr),
    suppressGC_(false)
{
    WH_ASSERT(runtime != nullptr);
    WH_ASSERT(hatchery != nullptr);
    WH_ASSERT(tenured != nullptr);

    tenuredList_.addSlab(tenured);
}

Runtime *
ThreadContext::runtime() const
{
    return runtime_;
}

Slab *
ThreadContext::hatchery() const
{
    return hatchery_;
}

Slab *
ThreadContext::nursery() const
{
    return nursery_;
}

Slab *
ThreadContext::tenured() const
{
    return tenured_;
}

const SlabList &
ThreadContext::tenuredList() const
{
    return tenuredList_;
}

SlabList &
ThreadContext::tenuredList()
{
    return tenuredList_;
}

RunContext *
ThreadContext::activeRunContext() const
{
    return activeRunContext_;
}

RootBase *
ThreadContext::roots() const
{
    return roots_;
}

bool
ThreadContext::suppressGC() const
{
    return suppressGC_;
}

void
ThreadContext::addRunContext(RunContext *runcx)
{
    WH_ASSERT(runcx->threadContext() == this);
    WH_ASSERT(runcx->next_ == nullptr);
    runcx->next_ = runContextList_;
    runContextList_ = runcx;
}


//
// AllocationContext
//

AllocationContext::AllocationContext(RunContext *cx, Slab *slab)
  : cx_(cx), slab_(slab)
{}


Value
AllocationContext::createString(uint32_t length, const uint8_t *bytes)
{
    // Check for integer index.
    int32_t idxVal = Value::ImmediateIndexValue(length, bytes);
    if (idxVal >= 0)
        return Value::ImmIndexString(idxVal);

    // Check if fits in immediate.
    if (length < Value::ImmString8MaxLength)
        return Value::ImmString8(length, bytes);

    return Value::HeapString(createSized<VM::LinearString>(length, bytes));
}

Value
AllocationContext::createString(uint32_t length, const uint16_t *bytes)
{
    // Check for integer index.
    int32_t idxVal = Value::ImmediateIndexValue(length, bytes);
    if (idxVal >= 0)
        return Value::ImmIndexString(idxVal);

    // Check if this is really an 8-bit immediate string in 16-bit clothes.
    if (length <= Value::ImmString8MaxLength) {
        bool isEightBit = true;
        for (unsigned i = 0; i < length; i++) {
            if (bytes[i] >= 0xFFu) {
                isEightBit = false;
                break;
            }
        }
        if (isEightBit) {
            uint8_t buf[Value::ImmString8MaxLength];
            for (unsigned i = 0; i < length; i++)
                buf[i] = bytes[i];
            return Value::ImmString8(length, buf);
        }
    }

    // Check if fits in 16-bit immediate string.
    if (length < Value::ImmString16MaxLength)
        return Value::ImmString16(length, bytes);

    return Value::HeapString(createSized<VM::LinearString>(length, bytes));
}

Value
AllocationContext::createNumber(double d)
{
    if (Value::IsImmediateNumber(d))
        return Value::Number(d);

    return Value::HeapDouble(create<VM::HeapDouble>(d));
}


VM::Tuple *
AllocationContext::createTuple(uint32_t count, const Value *vals)
{
    return createSized<VM::Tuple>(count * sizeof(Value), vals);
}


VM::Tuple *
AllocationContext::createTuple(uint32_t size)
{
    return createSized<VM::Tuple>(size * sizeof(Value));
}


//
// RunContext
//

RunContext::RunContext(ThreadContext *threadContext)
  : threadContext_(threadContext),
    next_(nullptr),
    hatchery_(threadContext_->hatchery()),
    topStackFrame_(nullptr),
    suppressGC_(threadContext_->suppressGC())
{
    threadContext_->addRunContext(this);
}

ThreadContext *
RunContext::threadContext() const
{
    return threadContext_;
}

Runtime *
RunContext::runtime() const
{
    return threadContext_->runtime();
}

Slab *
RunContext::hatchery() const
{
    WH_ASSERT(hatchery_ == threadContext_->hatchery());
    return hatchery_;
}

bool
RunContext::suppressGC() const
{
    return suppressGC_;
}

void
RunContext::makeActive()
{
    WH_ASSERT_IF(threadContext_->activeRunContext_ == this,
                 threadContext_->hatchery_ == hatchery_);
    if (threadContext_->activeRunContext_ != this) {
        threadContext_->activeRunContext_ = this;

        // Sync hatchery and suppressGC state.
        hatchery_ = threadContext_->hatchery();
        suppressGC_ = threadContext_->suppressGC();
    }
}

void
RunContext::registerTopStackFrame(VM::StackFrame *topStackFrame)
{
    // No stack frame should have been registered yet.
    WH_ASSERT(topStackFrame_ == nullptr);
    topStackFrame_ = topStackFrame;
}

AllocationContext
RunContext::inHatchery()
{
    return AllocationContext(this, hatchery_);
}

AllocationContext
RunContext::inTenured()
{
    return AllocationContext(this, threadContext_->tenured());
}


} // namespace Whisper


#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "slab.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "rooting_inlines.hpp"
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
// AllocationContext
//

AllocationContext::AllocationContext(ThreadContext *cx, Slab *slab)
  : cx_(cx), slab_(slab)
{}


bool
AllocationContext::createString(uint32_t length, const uint8_t *bytes,
                                Value &output)
{
    // Check for integer index.
    int32_t idxVal = Value::ImmediateIndexValue(length, bytes);
    if (idxVal >= 0) {
        output = Value::ImmIndexString(idxVal);
        return true;
    }

    // Check if fits in immediate.
    if (length < Value::ImmString8MaxLength) {
        output = Value::ImmString8(length, bytes);
        return true;
    }

    VM::LinearString *str = createSized<VM::LinearString>(length, bytes);
    if (!str)
        return false;

    output = Value::HeapString(str);
    return true;
}

bool
AllocationContext::createString(uint32_t length, const uint16_t *bytes,
                                Value &output)
{
    // Check for integer index.
    int32_t idxVal = Value::ImmediateIndexValue(length, bytes);
    if (idxVal >= 0) {
        output = Value::ImmIndexString(idxVal);
        return true;
    }

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
            output = Value::ImmString8(length, buf);
            return true;
        }
    }

    // Check if fits in 16-bit immediate string.
    if (length < Value::ImmString16MaxLength) {
        output = Value::ImmString16(length, bytes);
        return true;
    }

    VM::LinearString *str = createSized<VM::LinearString>(length, bytes);
    if (!str)
        return false;
        
    output = Value::HeapString(str);
    return true;
}

bool
AllocationContext::createNumber(double d, Value &value)
{
    if (Value::IsImmediateNumber(d)) {
        value = Value::Number(d);
        return true;
    }

    VM::HeapDouble *heapDouble = create<VM::HeapDouble>(d);
    if (!heapDouble)
        return false;

    value = Value::HeapDouble(heapDouble);
    return true;
}


bool
AllocationContext::createTuple(const VectorRoot<Value> &vals,
                               VM::Tuple *&output)
{
    output = createSized<VM::Tuple>(vals.size() * sizeof(Value), &vals.ref(0));
    if (!output)
        return false;
    return true;
}


bool
AllocationContext::createTuple(uint32_t size, VM::Tuple *&output)
{
    output = createSized<VM::Tuple>(size * sizeof(Value));
    if (!output)
        return false;
    return true;
}


//
// ThreadContext
//

/*static*/ unsigned int
ThreadContext::NewRandSeed()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    unsigned int result = tv.tv_sec * tv.tv_usec;

    // Handle low-resolution time (no multiples of 2, 5, or 10)
    while (result & 1)
        result >>= 1;
    while (result % 5 == 0)
        result /= 5;

    return result;
}

ThreadContext::ThreadContext(Runtime *runtime, Slab *hatchery, Slab *tenured)
  : runtime_(runtime),
    hatchery_(hatchery),
    nursery_(nullptr),
    tenured_(tenured),
    tenuredList_(),
    activeRunContext_(nullptr),
    runContextList_(nullptr),
    roots_(nullptr),
    suppressGC_(false),
    randSeed_(NewRandSeed()),
    stringTable_()
{
    WH_ASSERT(runtime != nullptr);
    WH_ASSERT(hatchery != nullptr);
    WH_ASSERT(tenured != nullptr);

    tenuredList_.addSlab(tenured);
    stringTable_.initialize(this);
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

void
ThreadContext::removeRunContext(RunContext *runcx)
{
    WH_ASSERT(runcx->threadContext() == this);
    WH_ASSERT(runcx->next_ == nullptr);
    bool found = false;
    RunContext *prevCx = nullptr;
    for (RunContext *cx = runContextList_; cx != nullptr; cx = cx->next_) {
        if (cx == runcx) {
            found = true;
            if (prevCx) {
                WH_ASSERT(prevCx->next_ == cx);
                prevCx->next_ = cx->next_;
                cx->next_ = nullptr;

            } else {
                runContextList_ = cx->next_;
                cx->next_ = nullptr;
            }

            break;
        }
    }
    WH_ASSERT(found);
}


AllocationContext
ThreadContext::inHatchery()
{
    return AllocationContext(this, hatchery_);
}


AllocationContext
ThreadContext::inTenured()
{
    return AllocationContext(this, tenured_);
}

int
ThreadContext::randInt()
{
    return rand_r(&randSeed_);
}

StringTable &
ThreadContext::stringTable()
{
    return stringTable_;
}

const StringTable &
ThreadContext::stringTable() const
{
    return stringTable_;
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
    return AllocationContext(threadContext_, hatchery_);
}

AllocationContext
RunContext::inTenured()
{
    return AllocationContext(threadContext_, threadContext_->tenured());
}

StringTable &
RunContext::stringTable()
{
    return threadContext_->stringTable();
}

const StringTable &
RunContext::stringTable() const
{
    return threadContext_->stringTable();
}


} // namespace Whisper

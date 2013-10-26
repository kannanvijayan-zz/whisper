
#include <string.h>

#include "slab.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/stack_frame.hpp"
#include "vm/string.hpp"
#include "vm/double.hpp"

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
    Slab *hatchery = Slab::AllocateStandard();
    if (!hatchery)
        return "Could not allocate hatchery slab.";

    // Allocate the ThreadContext
    ThreadContext *ctx;
    try {
        ctx = new ThreadContext(this, hatchery);
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
// StringTable
//

StringTable::Query::Query(uint32_t length, const uint8_t *data)
  : is8Bit(true), length(length), data(data)
{}

StringTable::Query::Query(uint32_t length, const uint16_t *data)
  : is8Bit(false), length(length), data(data)
{}

StringTable::StringOrQuery::StringOrQuery(const VM::HeapString *str)
  : ptr(reinterpret_cast<uintptr_t>(str))
{}

StringTable::StringOrQuery::StringOrQuery(const Query *str)
  : ptr(reinterpret_cast<uintptr_t>(str) | 1)
{}

bool
StringTable::StringOrQuery::isHeapString() const
{
    return !(ptr & 1);
}

bool
StringTable::StringOrQuery::isQuery() const
{
    return ptr & 1;
}

const VM::HeapString *
StringTable::StringOrQuery::toHeapString() const
{
    WH_ASSERT(isHeapString());
    return reinterpret_cast<const VM::HeapString *>(ptr);
}

const StringTable::Query *
StringTable::StringOrQuery::toQuery() const
{
    WH_ASSERT(isQuery());
    return reinterpret_cast<const Query *>(ptr);
}

StringTable::Hash::Hash(StringTable *table)
  : table(table)
{}

/*
static size_t Primes[] = {
    70241, 70853, 541483, 399557,
    280913, 77641, 136309, 1116523,
    660559, 102673, 626963, 690721,
    402697, 233609, 1088273, 862501
};

template <typename CharT>
static size_t
HashString(uint32_t spoiler, uint32_t length, const CharT *data)
{
    // Start with spoiler.
    size_t perturb = spoiler;
    size_t result = 1;

    for (uint32_t i = 0; i < length; i++) {
        uint16_t ch = data[i];
        ch ^= (perturb & 0xFFFFu);
        size_t prime = Primes[perturb & 0xF];
        size_t flow = prime * ch;
        result <<= 16;
        result ^= flow;
        perturb >>= 4;
        perturb ^= flow;
    }
    return result;
}
*/

size_t
StringTable::Hash::operator ()(const StringOrQuery &str)
{
    return 0;
}


//
// ThreadContext
//

ThreadContext::ThreadContext(Runtime *runtime, Slab *hatchery)
  : runtime_(runtime),
    hatchery_(hatchery),
    nursery_(nullptr),
    tenured_(),
    activeRunContext_(nullptr),
    runContextList_(nullptr),
    roots_(nullptr),
    suppressGC_(false)
{}

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

const SlabList &
ThreadContext::tenured() const
{
    return tenured_;
}

SlabList &
ThreadContext::tenured()
{
    return tenured_;
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

VM::HeapString *
RunContext::createString(uint32_t length, const uint8_t *bytes)
{
    return createSized<VM::LinearString>(length, bytes);
}

VM::HeapString *
RunContext::createString(uint32_t length, const uint16_t *bytes)
{
    return createSized<VM::LinearString>(length, bytes);
}

Value
RunContext::createDouble(double d)
{
    if (Value::IsImmediateDouble(d))
        return Value::Double(d);

    return Value::HeapDouble(create<VM::HeapDouble>(d));
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


} // namespace Whisper


#include <string.h>

#include "slab.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"

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
    Slab *nursery = Slab::AllocateStandard();
    if (!nursery)
        return "Could not allocate nursery slab.";

    // Allocate the ThreadContext
    ThreadContext *ctx;
    try {
        ctx = new ThreadContext(this, nursery);
        threadContexts_.push_back(ctx);
    } catch (std::bad_alloc &err) {
        return "Could not allocate ThreadContext.";
    }

    // Associate the thread context with the thread.
    int error = pthread_setspecific(threadKey_, ctx);
    if (error) {
        delete ctx;
        Slab::Destroy(nursery);
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

ThreadContext::ThreadContext(Runtime *runtime, Slab *nursery)
  : runtime_(runtime),
    hatchery_(nullptr),
    nursery_(nursery),
    tenured_(),
    activeRunContext_(nullptr),
    roots_(nullptr)
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

RunContext
ThreadContext::makeRunContext()
{
    return RunContext(this);
}

//
// RunContext
//

RunContext::RunContext(ThreadContext *threadContext)
  : threadContext_(threadContext),
    hatchery_(threadContext_->hatchery())
{}

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

void
RunContext::makeActive()
{
    WH_ASSERT_IF(threadContext_->activeRunContext_ == this,
                 threadContext_->hatchery_ == hatchery_);
    if (threadContext_->activeRunContext_ != this) {
        threadContext_->activeRunContext_ = this;
        syncHatchery();
    }
}

void
RunContext::syncHatchery()
{
    WH_ASSERT(threadContext_->activeRunContext() == this);
    hatchery_ = threadContext_->hatchery();
}


} // namespace Whisper

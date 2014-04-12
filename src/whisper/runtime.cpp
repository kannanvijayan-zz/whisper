
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

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

const char *
Runtime::registerThread()
{
    WH_ASSERT(initialized_);
    WH_ASSERT(pthread_getspecific(threadKey_) == nullptr);

    // Create a new nursery slab.
    Slab *hatchery = Slab::AllocateStandard(GCGen::Hatchery);
    if (!hatchery)
        return "Could not allocate hatchery slab.";

    // Create initial tenured space slab.
    Slab *tenured = Slab::AllocateStandard(GCGen::Tenured);
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
    WH_ASSERT(hasThreadContext());
    return maybeThreadContext();
}


//
// AllocationContext
//

AllocationContext::AllocationContext(ThreadContext *cx, Slab *slab)
  : cx_(cx), slab_(slab)
{}


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
    localHolders_(nullptr),
    suppressGC_(false),
    randSeed_(NewRandSeed()),
    spoiler_((randInt() & 0xffffU) | ((randInt() & 0xffffU) << 16))
{
    WH_ASSERT(runtime != nullptr);
    WH_ASSERT(hatchery != nullptr);
    WH_ASSERT(tenured != nullptr);

    tenuredList_.addSlab(tenured);
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

void
ThreadContext::activate(RunContext *cx)
{
    WH_ASSERT(cx);
    WH_ASSERT_IF(activeRunContext_ == cx, hatchery_ == cx->hatchery_);
    if (activeRunContext_ != cx) {
        activeRunContext_ = cx;

        // Sync hatchery and suppressGC state.
        cx->hatchery_ = hatchery_;
        cx->suppressGC_ = suppressGC_;
    }
}

void
ThreadContext::deactivateCurrentRunContext()
{
    WH_ASSERT(activeRunContext_);
    activeRunContext_->hatchery_ = nullptr;
    activeRunContext_ = nullptr;
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

uint32_t
ThreadContext::spoiler() const
{
    return spoiler_;
}


//
// RunContext
//

RunContext::RunContext(ThreadContext *threadContext)
  : threadContext_(threadContext),
    next_(nullptr),
    hatchery_(threadContext_->hatchery()),
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

//
// RunActivationHelper
//

RunActivationHelper::RunActivationHelper(RunContext &cx)
  : threadCx_(cx.threadContext()),
    oldRunCx_(cx.threadContext()->activeRunContext())
#if defined(ENABLE_DEBUG)
    ,
    runCx_(&cx)
#endif
{
    cx.threadContext()->activate(&cx);
}

RunActivationHelper::~RunActivationHelper()
{
#if defined(ENABLE_DEBUG)
    WH_ASSERT(threadCx_->activeRunContext() == runCx_);
#endif
    if (oldRunCx_)
        threadCx_->activate(oldRunCx_);
    else
        threadCx_->deactivateCurrentRunContext();
}


} // namespace Whisper

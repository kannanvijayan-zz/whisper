
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "slab.hpp"
#include "spew.hpp"
#include "parser/tokenizer.hpp"
#include "parser/parser.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"
#include "vm/frame.hpp"

namespace Whisper {


void
InitializeRuntime()
{
    InitializeSpew();
    InitializeTokenizer();
}


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
    Slab *hatchery = Slab::AllocateStandard(Gen::Hatchery);
    if (!hatchery)
        return "Could not allocate hatchery slab.";
    AutoDestroySlab _cleanupHatchery(hatchery);

    // Create initial tenured space slab.
    Slab *tenured = Slab::AllocateStandard(Gen::Tenured);
    if (!tenured)
        return "Could not allocate tenured slab.";
    AutoDestroySlab _cleanupTenured(tenured);

    // Allocate the ThreadContext
    ThreadContext *ctx = nullptr;
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
        return "pthread_setspecific failed to set ThreadContext.";
    }

    // Steal the allocated slabs so they don't get destroyed.
    _cleanupHatchery.steal();
    _cleanupTenured.steal();

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
    locals_(nullptr),
    lastFrame_(nullptr),
    suppressGC_(false),
    randSeed_(NewRandSeed()),
    spoiler_((randInt() & 0xffffU) | ((randInt() & 0xffffU) << 16)),
    error_(RuntimeError::None)
{
    WH_ASSERT(runtime != nullptr);
    WH_ASSERT(hatchery != nullptr);
    WH_ASSERT(tenured != nullptr);

    tenuredList_.addSlab(tenured);
}

void
ThreadContext::pushLastFrame(VM::Frame *frame)
{
    WH_ASSERT(frame->caller() == lastFrame_);
    lastFrame_ = frame;
}

void
ThreadContext::popLastFrame()
{
    WH_ASSERT(lastFrame_ != nullptr);
    lastFrame_ = lastFrame_->caller();
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


} // namespace Whisper

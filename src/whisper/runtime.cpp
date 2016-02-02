
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
#include "vm/box.hpp"
#include "vm/wobject.hpp"
#include "vm/scope_object.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "name_pool.hpp"

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
  : threadContexts_(),
    immortalThreadContext_(nullptr),
    runtimeState_(nullptr),
    nextRtid_(1024)
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

    if (!makeImmortalThreadContext())
        return false;

    ThreadContext* cx = immortalThreadContext_;
    AllocationContext acx = immortalThreadContext_->inTenured();

    Local<VM::RuntimeState*> rtState(cx);
    if (!rtState.setResult(VM::RuntimeState::Create(acx)))
        return false;

    runtimeState_ = rtState.get();

    initialized_ = true;
    return true;
}

OkResult
Runtime::registerThread()
{
    WH_ASSERT(initialized_);
    WH_ASSERT(pthread_getspecific(threadKey_) == nullptr);

    // Create a new nursery slab.
    Slab* hatchery = Slab::AllocateStandard(Gen::Hatchery);
    if (!hatchery)
        return okFail("Could not allocate hatchery slab.");
    AutoDestroySlab _cleanupHatchery(hatchery);

    // Create initial tenured space slab.
    Slab* tenured = Slab::AllocateStandard(Gen::Tenured);
    if (!tenured)
        return okFail("Could not allocate tenured slab.");
    AutoDestroySlab _cleanupTenured(tenured);

    // Allocate the ThreadContext
    ThreadContext* ctx = nullptr;
    try {
        ctx = new ThreadContext(this, nextRtid_++, hatchery, tenured);
        threadContexts_.push_back(ctx);
    } catch (std::bad_alloc& err) {
        return okFail("Could not allocate ThreadContext.");
    }

    // Allocate a new global object for the thread.
    if (!ctx->initialize()) {
        delete ctx;
        return okFail("Failed to initialize thread context.");
    }

    // Associate the thread context with the thread.
    int error = pthread_setspecific(threadKey_, ctx);
    if (error) {
        delete ctx;
        return okFail("pthread_setspecific failed to set ThreadContext.");
    }

    // Steal the allocated slabs so they don't get destroyed.
    _cleanupHatchery.steal();
    _cleanupTenured.steal();

    return OkVal();
}

OkResult
Runtime::makeImmortalThreadContext()
{
    WH_ASSERT(!initialized_);
    WH_ASSERT(immortalThreadContext_ == nullptr);

    // The immortal thread context only has a tenured
    // generation.

    Slab* tenured = Slab::AllocateStandard(Gen::Immortal);
    if (!tenured)
        return okFail("Could not allocate tenured slab.");
    AutoDestroySlab _cleanupTenured(tenured);

    // Allocate the ThreadContext
    ThreadContext* ctx = nullptr;
    try {
        ctx = new ThreadContext(this, 0, nullptr, tenured);
        threadContexts_.push_back(ctx);
    } catch (std::bad_alloc& err) {
        return okFail("Could not allocate ThreadContext.");
    }

    // Steal the allocated tenured slab so it doesn't get destroyed.
    _cleanupTenured.steal();

    immortalThreadContext_ = ctx;

    return OkVal();
}

ThreadContext*
Runtime::maybeThreadContext()
{
    WH_ASSERT(initialized_);

    return reinterpret_cast<ThreadContext*>(pthread_getspecific(threadKey_));
}

bool
Runtime::hasThreadContext()
{
    WH_ASSERT(initialized_);

    return maybeThreadContext() != nullptr;
}

ThreadContext*
Runtime::threadContext()
{
    WH_ASSERT(initialized_);
    WH_ASSERT(hasThreadContext());
    return maybeThreadContext();
}


//
// AllocationContext
//

AllocationContext::AllocationContext(ThreadContext* cx, Slab* slab)
  : cx_(cx), slab_(slab)
{}


//
// RuntimeError
//

char const*
RuntimeErrorString(RuntimeError err)
{
    switch (err) {
      case RuntimeError::None:
        return "None";
      case RuntimeError::MemAllocFailed:
        return "MemAllocFailed";
      case RuntimeError::SyntaxParseFailed:
        return "SyntaxParseFailed";
      case RuntimeError::LookupFailed:
        return "LookupFailed";
      case RuntimeError::InternalError:
        return "InternalError";
      case RuntimeError::ExceptionRaised:
        return "ExceptionRaised";
      default:
        return "UNKNOWN";
    }
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

ThreadContext::ThreadContext(Runtime* runtime, uint32_t rtid,
                             Slab* hatchery, Slab* tenured)
  : runtime_(runtime),
    rtid_(rtid),
    hatchery_(hatchery),
    nursery_(nullptr),
    tenured_(tenured),
    tenuredList_(),
    locals_(nullptr),
    threadState_(nullptr),
    suppressGC_(false),
    randSeed_(NewRandSeed()),
    spoiler_((randInt() & 0xffffU) | ((randInt() & 0xffffU) << 16))
{
    WH_ASSERT(runtime != nullptr);

    if (tenured)
        tenuredList_.addSlab(tenured);
}

VM::GlobalScope*
ThreadContext::global() const
{
    WH_ASSERT(hasGlobal());
    return threadState()->global();
}

VM::Wobject*
ThreadContext::rootDelegate() const
{
    WH_ASSERT(hasRootDelegate());
    return threadState()->rootDelegate();
}

bool
ThreadContext::hasError() const
{
    return threadState()->hasError();
}

RuntimeError
ThreadContext::error() const
{
    return threadState()->error();
}

bool
ThreadContext::hasErrorString() const
{
    return threadState()->hasErrorString();
}

char const*
ThreadContext::errorString() const
{
    return threadState()->errorString();
}

bool
ThreadContext::hasErrorThing() const
{
    return threadState()->hasErrorThing();
}

VM::Box const&
ThreadContext::errorThing() const
{
    return threadState()->errorThing();
}

ErrorT_
ThreadContext::setError(RuntimeError error, char const* string,
                        VM::Box const& thing)
{
    threadState()->setError(error, string, thing);
    return ErrorVal();
}

ErrorT_
ThreadContext::setError(RuntimeError error, char const* string,
                        HeapThing* thing)
{
    threadState()->setError(error, string, thing);
    return ErrorVal();
}

ErrorT_
ThreadContext::setError(RuntimeError error, char const* string)
{
    return setError(error, string, VM::Box());
}

size_t
ThreadContext::formatError(char* buf, size_t bufSize)
{
    WH_ASSERT(hasError());
    if (hasErrorString()) {
        if (hasErrorThing()) {
            char thingStr[128];
            errorThing().snprint(thingStr, 128);
            /*
            if (errorThing()->isString()) {
                snprintf(thingStr, 128, "String (%s)",
                            errorThing()->to<VM::String>()->c_chars());
            } else {
                snprintf(thingStr, 128, "%s",
                            errorThing()->header().formatString());
            }
            */
            return snprintf(buf, bufSize, "%s: %s [%s]",
                            RuntimeErrorString(error()),
                            errorString(), thingStr);
        }

        return snprintf(buf, bufSize, "%s: %s",
                        RuntimeErrorString(error()),
                        errorString());
    }

    return snprintf(buf, bufSize, "%s", RuntimeErrorString(error()));
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

OkResult
ThreadContext::initialize()
{
    // Initialize the thread state.
    Local<VM::ThreadState*> threadState(this);
    if (!threadState.setResult(VM::ThreadState::Create(inTenured())))
        return ErrorVal();
    threadState_ = threadState.get();

    return OkVal();
}


} // namespace Whisper

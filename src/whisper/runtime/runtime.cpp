
#include <pthread.h>

#include "common.hpp"
#include "debug.hpp"

#include "runtime/runtime.hpp"

namespace Whisper {
namespace Runtime {


// XXX For now, just impement support for a single thread of execution. XXX

//
// Runtime
//

void
Runtime::enterThread(ThreadState &thr)
{
    WH_ASSERT(threadStates_.empty());
    threadStates_.push_back(&thr);
    thr.initialize(this);
}

void
Runtime::exitThread(ThreadState &thr)
{
    WH_ASSERT(threadStates_.size() == 1);
    WH_ASSERT(threadStates_[0] == &thr);
    WH_ASSERT(thr.runtime_ == this);
    WH_ASSERT(thr.rootList_ == nullptr);
    threadStates_.pop_back();
    thr.runtime_ = nullptr;
}


//
// ThreadState
//

static ThreadState *GLOBAL_THREADSTATE = nullptr;

ThreadState *
ThreadState::Current()
{
    WH_ASSERT(GLOBAL_THREADSTATE != nullptr);
    return GLOBAL_THREADSTATE;
}

ThreadState::ThreadState()
  : runtime_(nullptr),
    rootList_(nullptr),
    hatchery_(nullptr)
{
}

ThreadState::~ThreadState()
{
    WH_ASSERT(runtime_ == nullptr);
    WH_ASSERT(rootList_ == nullptr);
    WH_ASSERT(GLOBAL_THREADSTATE == this);
    GLOBAL_THREADSTATE = nullptr;
}

void
ThreadState::initialize(Runtime *runtime)
{
    WH_ASSERT(runtime_ == nullptr);
    WH_ASSERT(GLOBAL_THREADSTATE == nullptr);
    runtime_ = runtime;
    hatchery_ = Slab::AllocateStandard();
    WH_ASSERT(hatchery_ != nullptr);
    GLOBAL_THREADSTATE = this;
}


} // namespace Runtime
} // namespace Whisper

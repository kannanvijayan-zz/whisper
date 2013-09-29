#ifndef WHISPER__RUNTIME_HPP
#define WHISPER__RUNTIME_HPP

#include <vector>
#include <pthread.h>

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"

namespace Whisper {


//
// Runtime
//
// A runtime represents one instance of the engine.
// However, a runtime itself holds very little state outside of
// being a container for multiple related thread contexts.
//
// Not all threads need to have associated thread contexts,
// but every thread which wishes to interact with the runtime
// in a subtantial way must have an associated one.
//

class RootBase;
class Slab;
class ThreadContext;
class RunContext;

class Runtime
{
  friend class ThreadContext;
  friend class RunContext;
  private:
    // Every running thread uses a thread context.
    std::vector<ThreadContext *> threadContexts_;
    pthread_key_t threadKey_;

    // initialized flag.
    bool initialized_ = false;

    static constexpr size_t ErrorBufferSize = 64;
    char errorBuffer_[ErrorBufferSize];
    const char *error_ = nullptr;

  public:
    Runtime();
    ~Runtime();

    bool initialize();

    bool hasError() const;
    const char *error() const;

    const char *registerThread();

    ThreadContext *maybeThreadContext();
    bool hasThreadContext();
    ThreadContext *threadContext();
};


//
// ThreadContext
//
// Holds all relevant information for a thread to interact with a runtime.
// A thread context is typically not used directly within the engine.
// Instead, a sub-construct, the RunContext, is used.
//

class ThreadContext
{
  friend class Runtime;
  friend class RunContext;
  friend class RootBase;
  private:
    Runtime *runtime_;
    Slab *hatchery_;
    Slab *nursery_;
    SlabList tenured_;
    RunContext *activeRunContext_;
    RootBase *roots_;

  public:
    ThreadContext(Runtime *runtime, Slab *hatchery);

    Runtime *runtime() const;
    Slab *hatchery() const;
    Slab *nursery() const;
    const SlabList &tenured() const;
    SlabList &tenured();
    RunContext *activeRunContext() const;
    RootBase *roots() const;

    RunContext makeRunContext();
};


//
// RunContext
//
// Holds the actual execution context for some active piece of code.
// While idle RunContexts may exist, at any point in time there's
// at most one active RunContext for a given ThreadContext.
//
// This RunContext carries state relevant to activation, as well as
// the privilege levels of executing code.  It also holds copies of
// the hatchery Slab pointer from the ThreadContext, for quick
// access without an extra indirection.
//

class RunContext
{
  friend class Runtime;
  friend class ThreadContext;
  private:
    ThreadContext *threadContext_;
    Slab *hatchery_;

  public:
    RunContext(ThreadContext *threadContext);

    ThreadContext *threadContext() const;

    Runtime *runtime() const;

    Slab *hatchery() const;

    // Construct an object in the hatchery.
    // Optionally GC-ing if necessary.
    template <typename ObjT, typename... Args>
    inline ObjT *create(bool allowGC, Args... args);

    template <typename ObjT, typename... Args>
    inline ObjT *createSized(bool allowGC, uint32_t size, Args... args);

    void makeActive();

  private:
    void syncHatchery();

    // Allocate an object in the hatchery.  This takes an explicit
    // size because some objects are variable sized.
    // Return null if not enough space in hatchery.
    template <typename ObjT>
    inline uint8_t *allocate(uint32_t size);
};


} // namespace Whisper

#endif // WHISPER__RUNTIME_HPP

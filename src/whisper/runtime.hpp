#ifndef WHISPER__RUNTIME_HPP
#define WHISPER__RUNTIME_HPP

#include <vector>
#include <unordered_set>
#include <pthread.h>

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "value.hpp"

namespace Whisper {

class RootBase;
class Slab;
class ThreadContext;
class RunContext;

namespace VM {
    class StackFrame;
    class HeapString;
    class Tuple;
}

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
    Slab *tenured_;
    SlabList tenuredList_;
    RunContext *activeRunContext_;
    RunContext *runContextList_;
    RootBase *roots_;
    bool suppressGC_;

  public:
    ThreadContext(Runtime *runtime, Slab *hatchery, Slab *tenured);

    Runtime *runtime() const;
    Slab *hatchery() const;
    Slab *nursery() const;
    Slab *tenured() const;
    const SlabList &tenuredList() const;
    SlabList &tenuredList();
    RunContext *activeRunContext() const;
    RootBase *roots() const;
    bool suppressGC() const;

    void addRunContext(RunContext *cx);
};


//
// AllocationContext
//
// An allocation context encapsulates the notion of a particular
// memory space as well as set of allocation criteria.
//

class AllocationContext
{
  private:
    RunContext *cx_;
    Slab *slab_;

  public:
    AllocationContext(RunContext *cx, Slab *slab);

    template <typename ObjT, typename... Args>
    inline ObjT *create(Args... args);

    template <typename ObjT, typename... Args>
    inline ObjT *createSized(uint32_t size, Args... args);

    Value createString(uint32_t length, const uint8_t *bytes);
    Value createString(uint32_t length, const uint16_t *bytes);

    Value createNumber(double d);

    VM::Tuple *createTuple(uint32_t count, const Value *vals);
    VM::Tuple *createTuple(uint32_t size);

  private:
    // Allocate an object.  This takes an explicit size because some
    // objects are variable sized.  Return null if not enough space
    // in hatchery.
    template <typename ObjT>
    inline uint8_t *allocate(uint32_t size);
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
// All RunContexts for a given thread are linked together with an
// embedded singly linked list.
//

class RunContext
{
  friend class Runtime;
  friend class ThreadContext;

  private:
    ThreadContext *threadContext_;
    RunContext *next_;
    Slab *hatchery_;
    VM::StackFrame *topStackFrame_;
    bool suppressGC_;

  public:
    RunContext(ThreadContext *threadContext);

    ThreadContext *threadContext() const;
    Runtime *runtime() const;
    Slab *hatchery() const;
    bool suppressGC() const;

    void makeActive();

    void registerTopStackFrame(VM::StackFrame *topStackFrame);

    AllocationContext inHatchery();
    AllocationContext inTenured();
};


} // namespace Whisper

#endif // WHISPER__RUNTIME_HPP

#ifndef WHISPER__RUNTIME_HPP
#define WHISPER__RUNTIME_HPP

#include <vector>
#include <unordered_set>
#include <pthread.h>

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"

namespace Whisper {

class ThreadContext;
class RunContext;
class RunActivationHelper;

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

    static constexpr size_t ErrorBufferSize = 256;
    char errorBuffer_[ErrorBufferSize];
    const char *error_ = nullptr;

  public:
    Runtime();
    ~Runtime();

    bool initialize();

    inline bool hasError() const {
        return error_;
    }

    inline const char *error() const {
        WH_ASSERT(hasError());
        return error_;
    }

    const char *registerThread();

    ThreadContext *maybeThreadContext();
    bool hasThreadContext();
    ThreadContext *threadContext();
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
    ThreadContext *cx_;
    Slab *slab_;

  public:
    AllocationContext(ThreadContext *cx, Slab *slab);

    template <typename ObjT, typename... Args>
    inline ObjT *create(Args... args);

    template <typename ObjT, typename... Args>
    inline ObjT *createSized(uint32_t size, Args... args);

  private:
    template <bool Traced>
    inline uint8_t *allocate(uint32_t size, SlabAllocType typeTag);
};


//
// AllocationTypeTrait
//
// For an AllocationContext to allocate an object using |create|, it needs
// to be able to automatically determine the allocation type of the object
// from its C++ type.
//
// Classes which expect to be usable with the |create| method should provide
// a specialization for this struct that defines |TYPETAG| appropriately.
//
template <typename T>
struct AllocationTypeTagTrait
{
    // static constexpr SlabAllocType TYPETAG;
};


//
// AllocationTracedTrait
//
// For an AllocationContext to allocate an object, it needs to be able
// to automatically determine whether the object is traceable or not, as
// this determines whether the object will be allocated from the bottom
// or top of the slab.
//
// Classes which expect to be usable with the |create| method should provide
// a specialization for this struct that defines |TRACED| appropriately.
//
template <typename T>
struct AllocationTracedTrait
{
    // static constexpr bool TRACED;
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
  friend class RunActivationHelper;
  private:
    Runtime *runtime_;
    Slab *hatchery_;
    Slab *nursery_;
    Slab *tenured_;
    SlabList tenuredList_;
    RunContext *activeRunContext_;
    RunContext *runContextList_;
    bool suppressGC_;

    unsigned int randSeed_;
    uint32_t spoiler_;

    static unsigned int NewRandSeed();

  public:
    ThreadContext(Runtime *runtime, Slab *hatchery, Slab *tenured);

    Runtime *runtime() const;
    Slab *hatchery() const;
    Slab *nursery() const;
    Slab *tenured() const;
    const SlabList &tenuredList() const;
    SlabList &tenuredList();
    RunContext *activeRunContext() const;
    bool suppressGC() const;

    void addRunContext(RunContext *cx);
    void removeRunContext(RunContext *cx);

    void activate(RunContext *cx);
    void deactivateCurrentRunContext();

    AllocationContext inHatchery();
    AllocationContext inTenured();

    int randInt();

    uint32_t spoiler() const;
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
    bool suppressGC_;

  public:
    RunContext(ThreadContext *threadContext);

    ThreadContext *threadContext() const;
    Runtime *runtime() const;
    Slab *hatchery() const;
    bool suppressGC() const;

    AllocationContext inHatchery();
    AllocationContext inTenured();
};


//
// RunActivationHelper
//
// An RAII helper class that makes a run context active on construction,
// and deactivates it on destruction.
//
// Asserts constraints during each phase.
//

class RunActivationHelper
{
  private:
    ThreadContext *threadCx_;
    RunContext *oldRunCx_;
#if defined(ENABLE_DEBUG)
    RunContext *runCx_;
#endif

  public:
    RunActivationHelper(RunContext &cx);
    ~RunActivationHelper();
};


} // namespace Whisper

#endif // WHISPER__RUNTIME_HPP

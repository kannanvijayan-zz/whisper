#ifndef WHISPER__RUNTIME_HPP
#define WHISPER__RUNTIME_HPP

#include <vector>
#include <unordered_set>
#include <pthread.h>

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "result.hpp"
#include "gc/local.hpp"

namespace Whisper {

class ThreadContext;
class RunActivationHelper;

//
// InitializeRuntime
//
// Set up the runtime for execution.
//
void InitializeRuntime();

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
    inline Result<ObjT *> create(Args... args);

    template <typename ObjT, typename... Args>
    inline Result<ObjT *> createSized(uint32_t size, Args... args);

    ThreadContext *threadContext() const {
        return cx_;
    }

  private:
    template <bool Traced>
    inline uint8_t *allocate(uint32_t size, HeapFormat fmt);
};

//
// ThreadContext
//
// Holds all relevant information for a thread to interact with a runtime.
//
class ThreadContext
{
  friend class Runtime;
  friend class LocalBase;
  friend class RunActivationHelper;
  private:
    Runtime *runtime_;
    Slab *hatchery_;
    Slab *nursery_;
    Slab *tenured_;
    SlabList tenuredList_;
    LocalBase *locals_;
    bool suppressGC_;

    unsigned int randSeed_;
    uint32_t spoiler_;

    static unsigned int NewRandSeed();

  public:
    ThreadContext(Runtime *runtime, Slab *hatchery, Slab *tenured);

    inline Runtime *runtime() const {
        return runtime_;
    }

    inline Slab *hatchery() const {
        return hatchery_;
    }

    inline Slab *nursery() const {
        return nursery_;
    }

    inline Slab *tenured() const {
        return tenured_;
    }

    inline const SlabList &tenuredList() const {
        return tenuredList_;
    }

    inline SlabList &tenuredList() {
        return tenuredList_;
    }

    inline LocalBase *locals() const {
        return locals_;
    }

    inline bool suppressGC() const {
        return suppressGC_;
    }

    AllocationContext inHatchery();
    AllocationContext inTenured();

    int randInt();

    uint32_t spoiler() const;
};


} // namespace Whisper

#endif // WHISPER__RUNTIME_HPP

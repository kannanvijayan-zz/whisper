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

namespace VM {
    class Frame;
    class TerminalFrame;
    class RuntimeState;
    class ThreadState;
    class GlobalScope;
    class Wobject;
    class Box;
}


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
    std::vector<ThreadContext*> threadContexts_;
    pthread_key_t threadKey_;

    // Immortal thread context and things allocated
    // within it.
    ThreadContext* immortalThreadContext_;
    VM::RuntimeState* runtimeState_;

    static constexpr uint32_t MaxRtid = 0x7fffffffu;
    uint32_t nextRtid_;

    // initialized flag.
    bool initialized_ = false;

    static constexpr size_t ErrorBufferSize = 256;
    char errorBuffer_[ErrorBufferSize];
    char const* error_ = nullptr;

  public:
    Runtime();
    ~Runtime();

    bool initialize();

    inline bool hasError() const {
        return error_;
    }

    inline char const* error() const {
        WH_ASSERT(hasError());
        return error_;
    }

    OkResult registerThread();

    ThreadContext* maybeThreadContext();
    bool hasThreadContext();
    ThreadContext* threadContext();

    VM::RuntimeState* runtimeState() const {
      return runtimeState_;
    }

  private:
    OkResult makeImmortalThreadContext();

    OkResult okFail(char const* error) {
        error_ = error;
        return OkResult::Error();
    }

    Maybe<uint32_t> nextRtid() {
        if (nextRtid_ >= MaxRtid)
            return Maybe<uint32_t>::None();
        return Maybe<uint32_t>::Some(nextRtid_++);
    }
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
    ThreadContext* cx_;
    Gen gen_;
    Slab* slab_;

  public:
    AllocationContext(ThreadContext* cx, Gen gen, Slab* slab);

    template <typename ObjT, typename... Args>
    inline Result<ObjT*> create(Args const&... args);

    template <typename ObjT, typename... Args>
    inline Result<ObjT*> createSized(uint32_t size, Args const&... args);

    ThreadContext* threadContext() const {
        return cx_;
    }

  private:
    template <bool Traced>
    inline uint8_t* allocate(uint32_t size, HeapFormat fmt);
};

//
// RuntimeError
//
// All possible error variants.
//
enum class RuntimeError
{
    None,
    MemAllocFailed,
    SyntaxParseFailed,
    LookupFailed,
    InternalError,
    ExceptionRaised
};

char const* RuntimeErrorString(RuntimeError err);

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
    Runtime* runtime_;
    uint32_t rtid_;

    Slab* hatchery_;
    SlabList hatcheryList_;

    Slab* nursery_;
    SlabList nurseryList_;

    Slab* tenured_;
    SlabList tenuredList_;
    LocalBase* locals_;
    VM::ThreadState* threadState_;
    bool suppressGC_;

    unsigned int randSeed_;
    uint32_t spoiler_;

    static unsigned int NewRandSeed();

  public:
    ThreadContext(Runtime* runtime, uint32_t rtid,
                  Slab* hatchery, Slab* tenured);

    Runtime* runtime() const {
        return runtime_;
    }

   uint32_t rtid() const {
        return rtid_;
    }

    Slab* hatchery() const {
        return hatchery_;
    }

    Slab* nursery() const {
        return nursery_;
    }

    Slab* tenured() const {
        return tenured_;
    }

    SlabList const& tenuredList() const {
        return tenuredList_;
    }

    SlabList& tenuredList() {
        return tenuredList_;
    }

    LocalBase* locals() const {
        return locals_;
    }

    VM::RuntimeState* runtimeState() const {
      return runtime_->runtimeState();
    }

    bool hasThreadState() const {
        return threadState_ != nullptr;
    }
    VM::ThreadState* threadState() const {
        WH_ASSERT(hasThreadState());
        return threadState_;
    }

    bool hasGlobal() const {
        return hasThreadState();
    }
    VM::GlobalScope* global() const;

    bool hasRootDelegate() const {
        return hasThreadState();
    }
    VM::Wobject* rootDelegate() const;

    bool suppressGC() const {
        return suppressGC_;
    }

    bool hasError() const;
    RuntimeError error() const;

    bool hasErrorString() const;
    char const* errorString() const;

    bool hasErrorThing() const;
    VM::Box const& errorThing() const;

    ErrorT_ setError(RuntimeError error, char const* string,
                     VM::Box const& thing);

    ErrorT_ setError(RuntimeError error, char const* string,
                     HeapThing* thing);

    ErrorT_ setError(RuntimeError error, char const* string);

    ErrorT_ setError(RuntimeError error) {
        return setError(error, nullptr);
    }
    template <typename T>
    ErrorT_ setError(RuntimeError error, char const* string, T* thing) {
        return setError(error, string, HeapThing::From(thing));
    }

    ErrorT_ setInternalError(char const* string) {
        return setError(RuntimeError::InternalError, string, nullptr);
    }

    size_t formatError(char* buf, size_t bufSize);

    AllocationContext inHatchery();
    AllocationContext inTenured();

    Slab* allocateStandardSlab(Gen gen);

    int randInt();

    uint32_t spoiler() const;

  private:
    OkResult initialize();
};


} // namespace Whisper

#endif // WHISPER__RUNTIME_HPP

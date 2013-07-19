#ifndef WHISPER__RUNTIME_HPP
#define WHISPER__RUNTIME_HPP

#include <vector>
#include <pthread.h>

#include "common.hpp"
#include "debug.hpp"
#include "vm/heap_thing.hpp"

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

    inline bool hasError() const {
        return error_ != nullptr;
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
  private:
    Runtime *runtime_;
    Slab *hatchery_;
    Slab *nursery_;
    SlabList tenured_;
    RunContext *activeRunContext_;

  public:
    ThreadContext(Runtime *runtime, Slab *nursery)
      : runtime_(runtime),
        hatchery_(nullptr),
        nursery_(nursery),
        tenured_(),
        activeRunContext_(nullptr)
    {}

    inline Runtime *runtime() const {
        return runtime_;
    }

    inline Slab *hatchery() const {
        return hatchery_;
    }

    inline Slab *nursery() const {
        return nursery_;
    }

    inline const SlabList &tenured() const {
        return tenured_;
    }

    inline SlabList &tenured() {
        return tenured_;
    }

    inline RunContext *activeRunContext() const {
        return activeRunContext_;
    }

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
    inline RunContext(ThreadContext *threadContext)
      : threadContext_(threadContext),
        hatchery_(threadContext_->hatchery())
    {}

    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline Runtime *runtime() const {
        return threadContext_->runtime();
    }

    inline Slab *hatchery() const {
        WH_ASSERT(hatchery_ == threadContext_->hatchery());
        return hatchery_;
    }

    // Construct an object in the hatchery.
    // Optionally GC-ing if necessary.
    template <typename ObjT, typename... Args>
    inline ObjT *create(bool allowGC, Args... args) {
        return create<ObjT, Args...>(allowGC, sizeof(ObjT), args...);
    }

    template <typename ObjT, typename... Args>
    inline ObjT *create(bool allowGC, uint32_t size, Args... args) {
        // Allocate the space for the object.
        uint8_t *mem = allocate<ObjT::Type>(size);
        if (!mem) {
            if (!allowGC)
                return nullptr;

            WH_ASSERT(!"GC infrastructure.");
            return nullptr;
        }

        // Figure out the card number.
        uint32_t cardNo = hatchery_->calculateCardNumber(mem);

        // Initialize the object using HeapThingWrapper, and
        // return it.
        typedef VM::HeapThingWrapper<ObjT, ObjT::Type> WrappedType;
        WrappedType *wrapped = new (mem) WrappedType(cardNo, size, args...);
        return wrapped->payloadPointer();
    }

    void makeActive();

  private:
    inline void syncHatchery() {
        WH_ASSERT(threadContext_->activeRunContext() == this);
        hatchery_ = threadContext_->hatchery();
    }

    // Allocate an object in the hatchery.  This takes an explicit
    // size because some objects are variable sized.
    // Return null if not enough space in hatchery.
    template <VM::HeapType ObjType>
    inline uint8_t *allocate(uint32_t size) {
        // Add HeaderSize to size.
        uint32_t allocSize = size + VM::HeapThing::HeaderSize;
        allocSize = AlignIntUp<uint32_t>(allocSize, Slab::AllocAlign);

        // Track whether to allocate from top or bottom.
        bool headAlloc = VM::HeapTypeTraits<ObjType>::Traced;

        // Allocate the space.
        return headAlloc ? hatchery_->allocateHead(allocSize)
                         : hatchery_->allocateTail(allocSize);
    }
};


} // namespace Whisper

#endif // WHISPER__RUNTIME_HPP

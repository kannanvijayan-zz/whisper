#ifndef WHISPER__RUNTIME__RUNTIME_HPP
#define WHISPER__RUNTIME__RUNTIME_HPP

#include <vector>

#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"

#include "runtime/rooting.hpp"
#include "runtime/heap_thing.hpp"

namespace Whisper {
namespace Runtime {


class ThreadState;
class AutoInitThreadState;
class BaseRootPtr;

//
// A runtime describes one isolated program execution consisting
// of one or more threads.
//
class Runtime
{
  friend class ThreadState;

  private:
    std::vector<ThreadState *> threadStates_;

  public:
    Runtime() : threadStates_() {}

    inline uint32_t numThreads() const {
        return threadStates_.size();
    }

    inline ThreadState *threadState(uint32_t threadno) const {
        WH_ASSERT(threadno < numThreads());
        return threadStates_[threadno];
    }

    void enterThread(ThreadState &thr);
    void exitThread(ThreadState &thr);
};

//
// Stores the state for a single thread of execution.
//
class ThreadState
{
  friend class BaseRootPtr;
  friend class Runtime;
  friend class AutoInitThreadState;

  public:
    static ThreadState *Current();

  private:
    Runtime *runtime_;
    BaseRootPtr *rootList_;
    Slab *hatchery_;

  public:
    ThreadState();
    ~ThreadState();

    inline bool isInitialized() const {
        return runtime_ != nullptr;
    }

    void initialize(Runtime *runtime);

    inline Runtime *runtime() const {
        WH_ASSERT(isInitialized());
        return runtime_;
    }

    template <typename T, typename... ARGS>
    inline T *create(ARGS... args) {
        static_assert(std::is_base_of<HeapThing, T>::value,
                      "Parameter type must inherit from HeapThing.");
        uint8_t *space = allocate(sizeof(T), HeapThingTraits<T>::TERMINAL);
        uint8_t *thing = space + sizeof(word_t);

        uint32_t cardNo = hatchery_->calculateCardNumber(thing);
        new (space) HeapThingHeader(cardNo, HeapThingTraits<T>::HEAP_TYPE);
        return new (thing) T(std::forward(args)...);
    }

    template <typename T, typename... ARGS>
    inline T *createSized(size_t size, ARGS... args) {
        static_assert(std::is_base_of<HeapThing, T>::value,
                      "Parameter type must inherit from HeapThing.");
        WH_ASSERT(size >= sizeof(T));
        uint8_t *space = allocate(size, HeapThingTraits<T>::TERMINAL);
        uint8_t *thing = space + sizeof(word_t);

        uint32_t cardNo = hatchery_->calculateCardNumber(thing);
        new (space) HeapThingHeader(cardNo, HeapThingTraits<T>::HEAP_TYPE);
        return new (thing) T(std::forward(args)...);
    }

  private:
    inline uint8_t *allocate(uint32_t size, bool terminal) {
        uint32_t allocSize = size + sizeof(word_t);
        uint8_t *result = terminal ? hatchery_->allocateHead(allocSize)
                                   : hatchery_->allocateTail(allocSize);
        WH_ASSERT(result);
        return result;
    }
};

//
// Helper class to initialize and destroy a thread state.
//
class AutoInitThreadState
{
  private:
    ThreadState threadState_;

  public:
    AutoInitThreadState(Runtime *runtime)
      : threadState_()
    {
        runtime->enterThread(threadState_);
    }

    ~AutoInitThreadState() {
        threadState_.runtime_->exitThread(threadState_);
    }
};


//
// Rooted pointers into the heap.
//
class BaseRootPtr
{
  protected:
    ThreadState *threadState_;
    HeapThing *ptr_;
    BaseRootPtr *next_;

  public:
    inline BaseRootPtr(ThreadState *threadState, HeapThing *ptr)
      : threadState_(threadState), ptr_(ptr), next_(threadState->rootList_)
    {
        threadState_->rootList_ = this;
    }

    inline ~BaseRootPtr()
    {
        WH_ASSERT(threadState_->rootList_ == this);
        threadState_->rootList_ = next_;
    }

    inline bool isValid() const {
        return ptr_ != nullptr;
    }

    inline explicit operator bool() const {
        return isValid();
    }

    inline HeapThing *maybeGet() const {
        return ptr_;
    }

    inline HeapThing *get() const {
        WH_ASSERT(isValid());
        return maybeGet();
    }

    inline HeapThing &operator *() const {
        return *get();
    }

    inline HeapThing *operator ->() const {
        return get();
    }

    inline operator HeapThing *() const {
        return get();
    }
};


template <typename T>
class RootPtr : public BaseRootPtr
{
  static_assert(std::is_base_of<HeapThing, T>::value,
                "Parameter type must inherit from HeapThing.");

  public:
    inline RootPtr(ThreadState *threadState, T *ptr)
      : BaseRootPtr(threadState, ptr)
    {}

    inline T *maybeGet() const {
        return reinterpret_cast<T *>(this->BaseRootPtr::maybeGet());
    }

    inline T *get() const {
        return reinterpret_cast<T *>(this->BaseRootPtr::get());
    }

    inline T &operator *() const {
        return reinterpret_cast<T &>(this->BaseRootPtr::operator *());
    }

    inline T *operator ->() const {
        return reinterpret_cast<T *>(this->BaseRootPtr::operator ->());
    }

    inline operator T *() const {
        return reinterpret_cast<T *>(this->BaseRootPtr::operator T *());
    }
};



} // namespace Runtime
} // namespace Whisper

#endif // WHISPER__RUNTIME__RUNTIME_HPP

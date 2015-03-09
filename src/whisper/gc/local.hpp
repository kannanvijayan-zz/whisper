#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

namespace Whisper {

class ThreadContext;
class RunContext;
class AllocationContext;

//
// GC::LocalBase
//
// Untyped base class for stack-root holders.  This class provides the
// basic plumbing for all stack-rooted structures: a constructor that
// registers the instance with a ThreadContext, and a destructor that
// de-registers it.
//

namespace GC {

class LocalBase
{
  protected:
    ThreadContext *threadContext_;
    LocalBase *next_;
    AllocHeader header_;

    inline LocalBase(const LocalBase &other);
    inline LocalBase(ThreadContext *threadContext, AllocFormat format);
    inline LocalBase(RunContext *runContext, AllocFormat format);
    inline LocalBase(AllocationContext &acx, AllocFormat format);
    inline ~LocalBase();

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline LocalBase *next() const {
        return next_;
    }

    inline AllocFormat format() const {
        return header_.format();
    }

    template <typename Scanner>
    void Scan(Scanner &scanner, void *start, void *end) const;

    template <typename Updater>
    void Update(Updater &updater, void *start, void *end);

  private:
    void *dataAfter() {
        return reinterpret_cast<uint8_t *>(this) + sizeof(this);
    }
    const void *dataAfter() const {
        return reinterpret_cast<const uint8_t *>(this) + sizeof(this);
    }
};

} // namespace GC

template <typename T> class MutHandle;
template <typename T> class Handle;


//
// Local
//
// Checked holder class that refers to a stack-rooted
// structure.
//

template <typename T>
class Local : public GC::LocalBase
{
    static_assert(GC::StackTraits<T>::Specialized,
                  "GC::StackTraits<T> not specialized.");

    typedef typename GC::DerefTraits<T>::Type DerefType;

  private:
    T val_;

  public:
    inline Local(const Local<T> &other)
      : GC::LocalBase(other),
        val_(other.val_)
    {}
    inline Local(Local<T> &&other)
      : GC::LocalBase(other),
        val_(std::move(other.val_))
    {}

    inline Local(ThreadContext *threadContext)
      : GC::LocalBase(threadContext, GC::StackTraits<T>::Format),
        val_()
    {}
    inline Local(ThreadContext *threadContext, const T &val)
      : GC::LocalBase(threadContext, GC::StackTraits<T>::Format),
        val_(val)
    {}
    inline Local(ThreadContext *threadContext, T &&val)
      : GC::LocalBase(threadContext, GC::StackTraits<T>::Format),
        val_(std::move(val))
    {}

    inline Local(RunContext *runContext)
      : GC::LocalBase(runContext, GC::StackTraits<T>::Format),
        val_()
    {}
    inline Local(RunContext *runContext, const T &val)
      : GC::LocalBase(runContext, GC::StackTraits<T>::Format),
        val_(val)
    {}
    inline Local(RunContext *runContext, T &&val)
      : GC::LocalBase(runContext, GC::StackTraits<T>::Format),
        val_(std::move(val))
    {}

    inline const T &get() const {
        return reinterpret_cast<const T &>(val_);
    }
    inline T &get() {
        return reinterpret_cast<T &>(val_);
    }

    inline void set(const T &ref) {
        val_ = ref;
    }
    inline void set(T &&ref) {
        val_ = ref;
    }

    inline const T *address() const {
        return &val_;
    }
    inline T *address() {
        return &val_;
    }

    inline operator const T &() const {
        return get();
    }
    inline operator T &() {
        return get();
    }

    inline const T *operator &() const {
        return address();
    }
    inline T *operator &() {
        return address();
    }

    inline const T &operator =(const T &ref) {
        set(ref);
        return ref;
    }
    inline const T &operator =(T &&ref) {
        this->set(ref);
        return ref;
    }

    inline const DerefType *operator ->() const {
        return GC::DerefTraits<T>::Deref(val_);
    }
    inline DerefType *operator ->() {
        return GC::DerefTraits<T>::Deref(val_);
    }

    inline MutHandle<T> mutHandle();
    inline Handle<T> handle() const;
};



} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

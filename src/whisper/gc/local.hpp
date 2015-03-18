#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/tracing.hpp"

namespace Whisper {

class ThreadContext;
class AllocationContext;

// LocalBase
// ---------
//
// Untyped base class for stack-root holders.  This class provides the
// basic plumbing for all stack-rooted structures: a constructor that
// registers the instance with a ThreadContext, and a destructor that
// de-registers it.
class LocalBase
{
  protected:
    ThreadContext *threadContext_;
    LocalBase *next_;
    StackHeader header_;

    inline LocalBase(const LocalBase &other) = delete;
    inline LocalBase(LocalBase &&other) = delete;
    inline LocalBase(ThreadContext *threadContext,
                     StackFormat format, uint32_t size);
    inline LocalBase(const AllocationContext &acx,
                     StackFormat format, uint32_t size);
    inline ~LocalBase();

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline LocalBase *next() const {
        return next_;
    }

    inline StackFormat format() const {
        return header_.format();
    }

    const StackThing *stackThing() const {
        return reinterpret_cast<const StackThing *>(dataAfter());
    }
    StackThing *stackThing() {
        return reinterpret_cast<StackThing *>(dataAfter());
    }

    template <typename Scanner>
    void scan(Scanner &scanner, void *start, void *end) const;

    template <typename Updater>
    void update(Updater &updater, void *start, void *end);

  private:
    void *dataAfter() {
        return reinterpret_cast<uint8_t *>(&header_) + sizeof(StackHeader);
    }
    const void *dataAfter() const {
        return reinterpret_cast<const uint8_t *>(&header_)
                + sizeof(StackHeader);
    }
};

template <typename T> class MutHandle;
template <typename T> class Handle;


// Local
// -----
//
// Checked holder class that refers to a stack-rooted
// structure.
template <typename T>
class Local : public LocalBase
{
    static_assert(alignof(T) <= 8, "Bad alignment for stack-rooted type.");
    static_assert(StackTraits<T>::Specialized,
                  "StackTraits<T> not specialized.");

    typedef typename DerefTraits<T>::Type DerefType;
    typedef typename DerefTraits<T>::ConstType ConstDerefType;

  private:
    T val_;

  public:
    inline Local(const Local<T> &other) = delete;
    inline Local(Local<T> &&other) = delete;

    inline Local(ThreadContext *threadContext)
      : LocalBase(threadContext, StackTraits<T>::Format, sizeof(T)),
        val_()
    {}
    inline Local(ThreadContext *threadContext, const T &val)
      : LocalBase(threadContext, StackTraits<T>::Format, sizeof(T)),
        val_(val)
    {}
    inline Local(ThreadContext *threadContext, T &&val)
      : LocalBase(threadContext, StackTraits<T>::Format, sizeof(T)),
        val_(std::move(val))
    {}

    inline Local(const AllocationContext &acx)
      : LocalBase(acx, StackTraits<T>::Format, sizeof(T)),
        val_()
    {}
    inline Local(const AllocationContext &acx, const T &val)
      : LocalBase(acx, StackTraits<T>::Format, sizeof(T)),
        val_(val)
    {}
    inline Local(const AllocationContext &acx, T &&val)
      : LocalBase(acx, StackTraits<T>::Format, sizeof(T)),
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
        val_ = std::move(ref);
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
        set(std::move(ref));
        return ref;
    }
    inline const T &operator =(T &&ref) {
        this->set(ref);
        return ref;
    }

    inline ConstDerefType *operator ->() const {
        return DerefTraits<T>::Deref(val_);
    }
    inline DerefType *operator ->() {
        return DerefTraits<T>::Deref(val_);
    }

    inline MutHandle<T> mutHandle();
    inline Handle<T> handle() const;
};



} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

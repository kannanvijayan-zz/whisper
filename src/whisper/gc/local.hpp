#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

namespace Whisper {

class ThreadContext;

//
// LocalHolderBase
//
// Untyped base class for stack-root holders.
//

class LocalHolderBase
{
  protected:
    ThreadContext *threadContext_;
    LocalHolderBase *next_;
    AllocFormat format_;

    inline LocalHolderBase(ThreadContext *threadContext, AllocFormat format);

  public:
    inline ThreadContext *threadContext() const {
        return threadContext_;
    }

    inline LocalHolderBase *next() const {
        return next_;
    }

    inline AllocFormat format() const {
        return format_;
    }
};

//
// LocalHolder
//
// Checked holder class that refers to a stack-rooted
// thing.
//

template <typename T>
class LocalHolder : public LocalHolderBase
{
  private:
    T val_;

  public:
    template <typename... Args>
    inline LocalHolder(ThreadContext *threadContext, Args... args)
      : LocalHolderBase(threadContext, AllocTraits<T>::FORMAT),
        val_(std::forward<Args>(args)...)
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

    inline operator const T &() const {
        return get();
    }
    inline operator T &() {
        return get();
    }

    inline const T *operator &() const {
        return &(get());
    }
    inline T *operator &() {
        return &(get());
    }

    inline LocalHolder<T> &operator =(const T &ref) {
        set(ref);
        return *this;
    }
    inline LocalHolder<T> &operator =(T &&ref) {
        this->set(ref);
        return *this;
    }
};



} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

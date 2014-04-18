#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

namespace Whisper {

class ThreadContext;

//
// LocalBase
//
// Untyped base class for stack-root holders.  This class provides the
// basic plumbing for all stack-rooted structures: a constructor that
// registers the instance with a ThreadContext, and a destructor that
// de-registers it.
//

class LocalBase
{
  protected:
    ThreadContext *threadContext_;
    LocalBase *next_;
    AllocHeader header_;

    inline LocalBase(ThreadContext *threadContext, AllocFormat format);
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
    void SCAN(Scanner &scanner, void *start, void *end) const;

    template <typename Updater>
    void UPDATE(Updater &updater, void *start, void *end);

  private:
    void *dataAfter() {
        class X : public LocalBase {
          public:
            word_t w;
        };
        return &(reinterpret_cast<X *>(this)->w);
    }
    const void *dataAfter() const {
        class X : public LocalBase {
          public:
            word_t w;
        };
        return &(reinterpret_cast<const X *>(this)->w);
    }
};

//
// Local
//
// Checked holder class that refers to a stack-rooted
// structure.
//

template <typename T>
class Local : public LocalBase
{
    static_assert(AllocTraits<T>::SPECIALIZED,
                  "AllocTraits<T> not specialized.");

  private:
    T val_;

  public:
    template <typename... Args>
    inline Local(ThreadContext *threadContext, Args... args)
      : LocalBase(threadContext, AllocTraits<T>::FORMAT),
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

    inline const T &operator =(const T &ref) {
        set(ref);
        return ref;
    }
    inline const T &operator =(T &&ref) {
        this->set(ref);
        return ref;
    }
};



} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

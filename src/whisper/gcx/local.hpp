#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gcx/core.hpp"

namespace Whisper {

class ThreadContext;

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


//
// Local
//
// Checked holder class that refers to a stack-rooted
// structure.
//

template <typename T>
class Local : public GC::LocalBase
{
    static_assert(AllocTraits<T>::Specialized,
                  "AllocTraits<T> not specialized.");

  private:
    T val_;

  public:
    template <typename... Args>
    inline Local(ThreadContext *threadContext, Args... args)
      : GC::LocalBase(threadContext, AllocTraits<T>::FORMAT),
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

    inline const DerefTraits<T>::DerefType *operator &() const {
        return address();
    }
    inline DerefTraits<T>::DerefType *operator &() {
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

    inline const DerefTraits<T>::Type *operator ->() const {
        return DerefTraits<T>::Deref(val_);
    }
    inline DerefTraits<T>::Type *operator ->() {
        return DerefTraits<T>::Deref(val_);
    }
};



} // namespace GC
} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

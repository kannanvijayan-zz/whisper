#ifndef WHISPER__GC__LOCAL_HPP
#define WHISPER__GC__LOCAL_HPP

#include "common.hpp"
#include "debug.hpp"
#include "result.hpp"
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
    ThreadContext* threadContext_;
    LocalBase* next_;
    StackHeader header_;

    inline LocalBase(LocalBase const& other) = delete;
    inline LocalBase(LocalBase&& other) = delete;
    inline LocalBase(ThreadContext* threadContext,
                     StackFormat format,
                     uint32_t count,
                     uint32_t size);
    inline LocalBase(AllocationContext const& acx,
                     StackFormat format,
                     uint32_t count,
                     uint32_t size);
    inline ~LocalBase();

  public:
    inline ThreadContext* threadContext() const {
        return threadContext_;
    }

    inline LocalBase* next() const {
        return next_;
    }

    inline StackFormat format() const {
        return header_.format();
    }
    inline uint32_t count() const {
        return header_.count();
    }

    StackThing const* stackThing(uint32_t idx) const {
        WH_ASSERT(idx < count());
        return reinterpret_cast<StackThing const*>(dataAfter()) + idx;
    }
    StackThing* stackThing(uint32_t idx) {
        return reinterpret_cast<StackThing*>(dataAfter()) + idx;
    }

    template <typename Scanner>
    void scan(Scanner& scanner, void* start, void* end) const;

    template <typename Updater>
    void update(Updater& updater, void* start, void* end);

  protected:
    void* dataAfter() {
        return reinterpret_cast<uint8_t*>(&header_) + sizeof(StackHeader);
    }
    void const* dataAfter() const {
        return reinterpret_cast<uint8_t const*>(&header_)
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
    inline Local(Local<T> const& other) = delete;
    inline Local(Local<T>&& other) = delete;

    inline Local(ThreadContext* threadContext)
      : LocalBase(threadContext, StackTraits<T>::Format, 1, sizeof(T)),
        val_()
    {}
    inline Local(ThreadContext* threadContext, T const& val)
      : LocalBase(threadContext, StackTraits<T>::Format, 1, sizeof(T)),
        val_(val)
    {}
    inline Local(ThreadContext* threadContext, T&& val)
      : LocalBase(threadContext, StackTraits<T>::Format, 1, sizeof(T)),
        val_(std::move(val))
    {}

    inline Local(AllocationContext const& acx)
      : LocalBase(acx, StackTraits<T>::Format, 1, sizeof(T)),
        val_()
    {}
    inline Local(AllocationContext const& acx, T const& val)
      : LocalBase(acx, StackTraits<T>::Format, 1, sizeof(T)),
        val_(val)
    {}
    inline Local(AllocationContext const& acx, T&& val)
      : LocalBase(acx, StackTraits<T>::Format, 1, sizeof(T)),
        val_(std::move(val))
    {}

    inline T const& get() const {
        return reinterpret_cast<T const&>(val_);
    }
    inline T& get() {
        return reinterpret_cast<T&>(val_);
    }

    inline void set(T const& ref) {
        val_ = ref;
    }
    inline void set(T&& ref) {
        val_ = std::move(ref);
    }
    OkResult setResult(Result<T> const& result) {
        if (result.isError())
            return ErrorVal();
        val_ = result.value();
        return OkVal();
    }

    inline T const* address() const {
        return &val_;
    }
    inline T* address() {
        return &val_;
    }

    inline operator T const&() const {
        return get();
    }
    inline operator T&() {
        return get();
    }

    inline Local<T>& operator =(T const& ref) {
        this->set(ref);
        return *this;
    }
    inline Local<T>& operator =(T&& ref) {
        set(std::move(ref));
        return *this;
    }

    inline ConstDerefType* operator ->() const {
        return DerefTraits<T>::Deref(val_);
    }
    inline DerefType* operator ->() {
        return DerefTraits<T>::Deref(val_);
    }

    inline MutHandle<T> mutHandle();
    inline Handle<T> handle() const;
};

template <typename T, unsigned N> class LocalArrayBase;

// Specialization for LocalArrayBase<T, UINT32_MAX> behaves as a
// dynamically sized array.
//
// It cannot be constructed.
template <typename T>
class LocalArrayBase<T, UINT32_MAX> : public LocalBase
{
    static_assert(alignof(T) <= 8, "Bad alignment for stack-rooted type.");
    static_assert(StackTraits<T>::Specialized,
                  "StackTraits<T> not specialized.");

  private:
    typedef typename DerefTraits<T>::Type DerefType;
    typedef typename DerefTraits<T>::ConstType ConstDerefType;

    inline LocalArrayBase(LocalArrayBase const& other) = delete;
    inline LocalArrayBase(LocalArrayBase&& other) = delete;

    inline LocalArrayBase(ThreadContext* threadContext, uint32_t length)
      : LocalBase(threadContext, StackTraits<T>::Format, length, sizeof(T))
    {}
    inline LocalArrayBase(AllocationContext const& acx, uint32_t length)
      : LocalBase(acx, StackTraits<T>::Format, length, sizeof(T))
    {}

  public:
    inline uint32_t length() {
        return this->count();
    }

    inline T const& get(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return reinterpret_cast<T const&>(this->elementAt(idx));
    }
    inline T& get(uint32_t idx) {
        WH_ASSERT(idx < length());
        return reinterpret_cast<T&>(this->elementAt(idx));
    }

    inline void set(uint32_t idx, T const& ref) {
        WH_ASSERT(idx < length());
        this->elementAt(idx) = ref;
    }
    inline void set(uint32_t idx, T&& ref) {
        WH_ASSERT(idx < length());
        this->elementAt(idx) = std::move(ref);
    }
    OkResult setResult(uint32_t idx, Result<T> const& result) {
        WH_ASSERT(idx < length());
        if (result.isError())
            return ErrorVal();
        this->elementAt(idx) = result.value();
        return OkVal();
    }

    inline T const* address(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return &this->elementAt(idx);
    }
    inline T* address(uint32_t idx) {
        WH_ASSERT(idx < length());
        return &this->elementAt(idx);
    }

    inline operator T const*() {
        return address(0);
    }
    inline operator T*() {
        return address(0);
    }

    inline MutHandle<T> mutHandle(uint32_t idx);
    inline Handle<T> handle(uint32_t idx) const;

    inline MutHandle<T> operator[](uint32_t idx) {
        return mutHandle(idx);
    }
    inline Handle<T> operator[](uint32_t idx) const {
        return handle(idx);
    }

  private:
    T& elementAt(uint32_t idx) {
        WH_ASSERT(idx < count());
        return *(reinterpret_cast<T*>(dataAfter()) + idx);
    }
    T const& elementAt(uint32_t idx) const {
        WH_ASSERT(idx < count());
        return *(reinterpret_cast<T const*>(dataAfter()) + idx);
    }
};

// LocalArrayBase
// --------------
//
// Checked holder class that refers to an array of stack-rooted
// structures.
template <typename T, uint32_t N>
class LocalArrayBase : public LocalArrayBase<T, UINT32_MAX>
{
    static_assert(alignof(T) <= 8, "Bad alignment for stack-rooted type.");
    static_assert(StackTraits<T>::Specialized,
                  "StackTraits<T> not specialized.");
    static_assert(N < UINT32_MAX, "Array size too large.");

    typedef LocalArrayBase<T, UINT32_MAX> BaseType;

    typedef typename DerefTraits<T>::Type DerefType;
    typedef typename DerefTraits<T>::ConstType ConstDerefType;

  private:
    T vals_[N];

  public:
    inline LocalArrayBase(LocalArrayBase<T, N> const& other) = delete;
    inline LocalArrayBase(LocalArrayBase<T, N>&& other) = delete;

    inline LocalArrayBase(ThreadContext* threadContext)
      : BaseType(threadContext, StackTraits<T>::Format, N, sizeof(T)),
        vals_()
    {}
    inline LocalArrayBase(AllocationContext const& acx)
      : BaseType(acx, StackTraits<T>::Format, N, sizeof(T)),
        vals_()
    {}
};

template <typename T, uint32_t N=UINT32_MAX>
using LocalArray = LocalArrayBase<T, N>;


} // namespace Whisper

#endif // WHISPER__GC__LOCAL_HPP

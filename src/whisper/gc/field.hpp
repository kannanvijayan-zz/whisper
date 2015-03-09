#ifndef WHISPER__GC__FIELD_HPP
#define WHISPER__GC__FIELD_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/core.hpp"

namespace Whisper {


//
// GC::BaseField
//
// Base helper class for HeapField and StackField.
//
namespace GC {

class AllocThing;

template <typename T>
class BaseField
{
    static_assert(TraceTraits<T>::Specialized,
                  "TraceTraits has not been specialized for type.");
    typedef typename GC::DerefTraits<T>::Type DerefType;

  protected:
    T val_;

  public:
    inline BaseField()
      : val_()
    {}
    inline BaseField(const T &val)
      : val_(val)
    {}
    inline BaseField(T &&val)
      : val_(std::move(val))
    {}

    inline const T &get() const {
        return val_;
    }
    inline T &getRaw() {
        return val_;
    }
    inline const T *address() const {
        return &val_;
    }

    inline operator const T &() const {
        return get();
    }

    inline const T *operator &() const {
        return address();
    }

    inline const DerefType *operator ->() const {
        return GC::DerefTraits<T>::Deref(val_);
    }

    T &operator =(const BaseField<T> &other) = delete;

    template <typename Scanner>
    inline void scan(Scanner &scanner, const void *start, const void *end) const
    {
        TraceTraits<T>::Scan(scanner, val_, start, end);
    }

    template <typename Updater>
    inline void update(Updater &updater, const void *start, const void *end)
    {
        TraceTraits<T>::Update(updater, val_, start, end);
    }
};

} // namespace GC


//
// HeapField
//
// Holder for a traced object stored in a field on a heap object.
// The
//

template <typename T>
class HeapField : public GC::BaseField<T>
{
  public:
    inline HeapField()
      : GC::BaseField<T>()
    {}
    explicit inline HeapField(const T &val)
      : GC::BaseField<T>(val)
    {}
    explicit inline HeapField(T &&val)
      : GC::BaseField<T>(std::move(val))
    {}

    inline void notifySetPre(GC::AllocThing *container) {
        // TODO: Use TraceTraits to scan val_, and register any
        // existing pointers.
    }
    inline void notifySetPost(GC::AllocThing *container) {
        // TODO: Use TraceTraits to scan val_, and register any
        // new pointers.
    }

    template <typename AllocThingT>
    inline void set(const T &ref, AllocThingT *container) {
        notifySetPre(GC::AllocThing::From(container));
        this->val_ = ref;
        notifySetPost(GC::AllocThing::From(container));
    }

    template <typename AllocThingT>
    inline void set(T &&ref, AllocThingT *container) {
        notifySetPre(GC::AllocThing::From(container));
        this->val_ = ref;
        notifySetPost(GC::AllocThing::From(container));
    }

    template <typename AllocThingT, typename... Args>
    inline void init(AllocThingT *container, Args... args) {
        // Pre-notification not required as value is not initialized.
        new (&this->val_) T(std::forward<Args>(args)...);
        notifySetPost(GC::AllocThing::From(container));
    }

    template <typename AllocThingT>
    inline void destroy(AllocThingT *container) {
        notifySetPre(GC::AllocThing::From(container));
        this->val_.~T();
        // Post-notification not required as value is destroyed.
    }

    T &operator =(const HeapField<T> &other) = delete;
};


//
// StackField
//
// Holder for a traced object stored in a field on a stack object.
//

template <typename T>
class StackField : public GC::BaseField<T>
{
  public:
    inline StackField()
      : GC::BaseField<T>()
    {}
    explicit inline StackField(const T &val)
      : GC::BaseField<T>(val)
    {}
    explicit inline StackField(T &&val)
      : GC::BaseField<T>(std::move(val))
    {}

    template <typename AllocThingT>
    inline void set(const T &ref, AllocThingT *container) {
        GC::AllocThing::From(container);
        this->val_ = ref;
    }

    template <typename AllocThingT>
    inline void set(T &&ref, AllocThingT *container) {
        GC::AllocThing::From(container);
        this->val_ = ref;
    }

    template <typename AllocThingT, typename... Args>
    inline void init(AllocThingT *container, Args... args) {
        GC::AllocThing::From(container);
        new (&this->val_) T(std::forward<Args>(args)...);
    }

    template <typename AllocThingT>
    inline void destroy(AllocThingT *container) {
        GC::AllocThing::From(container);
        this->val_.~T();
    }

    T &operator =(const StackField<T> &other) {
        this->val_ = other.val_;
    }
};


//
// ChooseField<Type>
//
// Helper template that lets you choose based on a FieldType enum value
// between StackField and HeapField.
//

enum class FieldType { Stack, Heap };

template <FieldType T, typename T>
class ChooseField
{
    ChooseField() = delete;
};

template <typename T>
class ChooseField<FieldType::Stack, T>
{
    ChooseField() = delete;

    typedef StackField<T> Type;
};

template <typename T>
class ChooseField<FieldType::Heap, T>
{
    ChooseField() = delete;

    typedef HeapField<T> Type;
};


} // namespace Whisper

#endif // WHISPER__GC__FIELD_HPP

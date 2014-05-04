#ifndef WHISPER__GC__FIELD_HPP
#define WHISPER__GC__FIELD_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gcx/core.hpp"

namespace Whisper {


class SlabThing;


//
// GC::BaseField
//
// Base helper class for HeapField and StackField.
//
namespace GC {

template <typename T>
class BaseField
{
    static_assert(TraceTraits<T>::Specialized,
                  "TraceTraits has not been specialized for type.");

  protected:
    T val_;

  public:
    template <typename... Args>
    inline BaseField(Args... args)
      : val_(std::forward<Args>(args)...)
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

    T &operator =(const BaseField<T> &other) = delete;

    template <typename Scanner>
    inline void SCAN(Scanner &scanner, void *start, void *end) const
    {
        TraceTraits<T>::Scan(scanner, val_, start, end);
    }

    template <typename Updater>
    inline void UPDATE(Updater &updater, void *start, void *end) const
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
    template <typename... Args>
    inline HeapField(Args... args)
      : GC::BaseField<T>(std::forward<Args>(args)...)
    {}

    inline void notifySetPre(SlabThing *container) {
        // TODO: Use TraceTraits to scan val_, and register any
        // existing pointers.
    }
    inline void notifySetPost(SlabThing *container) {
        // TODO: Use TraceTraits to scan val_, and register any
        // new pointers.
    }

    inline void set(const T &ref, SlabThing *container) {
        notifySetPre(container);
        this->val_ = ref;
        notifySetPost(container);
    }
    inline void set(T &&ref, SlabThing *container) {
        notifySetPre(container);
        this->val_ = ref;
        notifySetPost(container);
    }

    template <typename SlabThingT, typename... Args>
    inline void init(SlabThingT *container, Args... args) {
        // Pre-notification not required as value is not initialized.
        new (&this->val_) T(std::forward<Args>(args)...);
        notifySetPost(container);
    }

    inline void destroy(SlabThing *container) {
        notifySetPre(container);
        this->val_.~T();
        // Post-notification not required as value is destroyed.
    }

    inline const DerefTraits<T>::Type *operator ->() const {
        return DerefTraits<T>::Deref(val_);
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
    template <typename... Args>
    inline StackField(Args... args)
      : GC::BaseField<T>(std::forward<Args>(args)...)
    {}

    inline void set(const T &ref, SlabThing *container) {
        this->val_ = ref;
    }
    inline void set(T &&ref, SlabThing *container) {
        this->val_ = ref;
    }

    template <typename SlabThingT, typename... Args>
    inline void init(SlabThingT *container, Args... args) {
        new (&this->val_) T(std::forward<Args>(args)...);
    }

    inline void destroy(SlabThing *container) {
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

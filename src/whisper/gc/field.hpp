#ifndef WHISPER__GC__FIELD_HPP
#define WHISPER__GC__FIELD_HPP

#include "common.hpp"
#include "debug.hpp"
#include "gc/tracing.hpp"

//
// FIELD STRUCTURES
// ================
//
// It is useful to be able to embed within traced objects
// types whose instances may hold references to heap objects.
//
// These fields instances must allow for their references to be
// scanned and updated, since they are embedded within
// heap and stack allocated structures.
//
// Since fields are embedded in their containing structure, the
// static type of the containing structure captures the static
// type of the field structure.  The GC will dynamically infer the
// static type of the containing structure (which is either on stack
// or on heap), and thus implicitly infer the static type of the field.
//
// Fields on stack structures behave differently from fields on heap
// structures.  For example, mutations of fields on stack-allocated
// objects require no additional bookkeeping, but mutations of fields
// on heap-allocated objects may trigger write barriers.
//
// To support these two designs, there are two field wrapper templates:
// StackField<T> and HeapField<T>.  StackField<T>s are meant to be contained
// within stack-allocated values, and HeapField<T>s are meant to be contained
// within heap-allocated values.
//
// To be able to be wrapped by StackField or HeapField, a type T must
// specialize FieldTraits<T>, and addictionally TraceTraits<T>.  See
// the TRACING section for more details.
//

namespace Whisper {


// FieldTraits
// -----------
//
//  FieldTraits contains the following definitions:
//
//      // Must be set true by all FieldTraits specializations.
//      static constexpr bool Specialized = true;
template <typename T>
struct FieldTraits
{
    FieldTraits() = delete;

    static constexpr bool Specialized = false;
};


// BaseField
// ---------
//
// Base helper class for HeapField and StackField.
//
template <typename T>
class BaseField
{
    static_assert(TraceTraits<T>::Specialized,
                  "TraceTraits has not been specialized for type.");
    typedef typename DerefTraits<T>::Type DerefType;
    typedef typename DerefTraits<T>::ConstType ConstDerefType;

  protected:
    T val_;

  public:
    inline BaseField()
      : val_()
    {}
    inline BaseField(T const& val)
      : val_(val)
    {}
    inline BaseField(T&& val)
      : val_(std::move(val))
    {}

    inline T const& get() const {
        return val_;
    }
    inline T& getRaw() {
        return val_;
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

    inline ConstDerefType* operator ->() const {
        return DerefTraits<T>::Deref(val_);
    }
    inline DerefType* operator ->() {
        return DerefTraits<T>::Deref(val_);
    }

    T& operator =(BaseField<T> const& other) = delete;

    template <typename Scanner>
    inline void scan(Scanner& scanner, void const* start, void const* end) const
    {
        TraceTraits<T>::Scan(scanner, val_, start, end);
    }

    template <typename Updater>
    inline void update(Updater& updater, void const* start, void const* end)
    {
        TraceTraits<T>::Update(updater, val_, start, end);
    }
};


// HeapField
// ---------
//
// Holder for a traced object stored in a field on a heap object.
// The
//

template <typename T>
class HeapField : public BaseField<T>
{
  public:
    inline HeapField()
      : BaseField<T>()
    {}
    explicit inline HeapField(T const& val)
      : BaseField<T>(val)
    {}
    explicit inline HeapField(T&& val)
      : BaseField<T>(std::move(val))
    {}

    template <typename HeapT>
    inline void notifySetPre(HeapT* container) {
        // TODO: Use TraceTraits to scan val_, and register any
        // existing pointers.
    }

    template <typename HeapT>
    inline void notifySetPost(HeapT* container) {
        // TODO: Use TraceTraits to scan val_, and register any
        // new pointers.
    }

    template <typename HeapT>
    inline void set(T const& ref, HeapT* container) {
        notifySetPre(container);
        this->val_ = ref;
        notifySetPost(container);
    }
    template <typename HeapT>
    inline void set(T&& ref, HeapT* container) {
        notifySetPre(container);
        this->val_ = ref;
        notifySetPost(container);
    }

    template <typename HeapT>
    inline void init(T const& val, HeapT* container) {
        // Pre-notification not required as value is not initialized.
        new (&this->val_) T(val);
        notifySetPost(container);
    }
    template <typename HeapT>
    inline void init(T&& val, HeapT* container) {
        // Pre-notification not required as value is not initialized.
        new (&this->val_) T(std::move(val));
        notifySetPost(container);
    }

    template <typename HeapT>
    inline void clear(T const& ref, HeapT* container) {
        notifySetPre(container);
        this->val_ = ref;
    }
    template <typename HeapT>
    inline void clear(T&& ref, HeapT* container) {
        notifySetPre(container);
        this->val_ = ref;
    }

    template <typename HeapT>
    inline void destroy(HeapT* container) {
        notifySetPre(container);
        this->val_.~T();
        // Post-notification not required as value is destroyed.
    }

    T& operator =(HeapField<T> const& other) = delete;
};


// StackField
// ----------
//
// Holder for a traced object stored in a field on a stack object.
//
template <typename T>
class StackField : public BaseField<T>
{
  public:
    inline StackField()
      : BaseField<T>()
    {}
    explicit inline StackField(T const& val)
      : BaseField<T>(val)
    {}
    explicit inline StackField(T&& val)
      : BaseField<T>(std::move(val))
    {}

    inline void set(T const& ref) {
        this->val_ = ref;
    }

    inline void set(T&& ref) {
        this->val_ = ref;
    }

    inline void init(T const& ref) {
        new (&this->val_) T(ref);
    }
    inline void init(T&& ref) {
        new (&this->val_) T(std::move(ref));
    }

    inline void destroy() {
        this->val_.~T();
    }

    T& operator =(StackField<T> const& other) {
        this->val_ = other.val_;
        return this->val_;
    }
    T& operator =(T const& other) {
        this->val_ = other.val_;
        return this->val_;
    }
    T& operator =(T&& other) {
        this->val_ = std::move(other.val_);
        return this->val_;
    }
};


} // namespace Whisper

#endif // WHISPER__GC__FIELD_HPP

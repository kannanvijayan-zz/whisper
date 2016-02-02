#ifndef WHISPER__VM__SLIST_HPP
#define WHISPER__VM__SLIST_HPP


#include "vm/core.hpp"

namespace Whisper {
namespace VM {


// Template to annotate types which are used as parameters to
// Slist<>, so that we can derive an HeapFormat for the slist of
// a particular type.
template <typename T>
struct SlistTraits
{
    SlistTraits() = delete;

    // Set to true for all specializations.
    static const bool Specialized = false;

    // Give an HeapFormat for an slist of type T.
    //
    // static const HeapFormat SlistFormat;

    // Give a map-back type T' to map the slist format back to
    // VM::Slist<T'>
    // typedef SomeType T;
};


//
// A singly-linked list.
//
template <typename T>
class Slist
{
    friend struct TraceTraits<Slist<T>>;

    // T must be a field type to be usable.
    static_assert(FieldTraits<T>::Specialized,
                  "Underlying type is not field-specialized.");
    static_assert(SlistTraits<T>::Specialized,
                  "Underlying type does not have an slist specialization.");

  private:
    HeapField<T> value_;
    HeapField<Slist<T>*> rest_;

  public:
    Slist() = delete;
    Slist(T const& value)
      : value_(value), rest_(nullptr)
    {}
    Slist(T const& value, Slist<T>* rest)
      : value_(value), rest_(rest)
    {}

    T const& value() const {
        return value_;
    }
    Slist<T>* rest() const {
        return rest_;
    }

    static inline Result<Slist<T>*> Create(AllocationContext acx,
                                           Handle<T> value)
    {
        return Create(acx, value, nullptr);
    }
    static inline Result<Slist<T>*> Create(AllocationContext acx,
                                           Handle<T> value,
                                           Handle<Slist<T>*> rest);

    inline uint32_t length() const;
};


} // namespace VM
} // namespace Whisper

// Include slist template specializations for describing slists to
// the GC.
#include "vm/slist_specializations.hpp"


namespace Whisper {
namespace VM {


template <typename T>
inline uint32_t
Slist<T>::length() const
{
    uint32_t length = 0;
    Slist<T> const* cur = this;
    while (cur != nullptr) {
        length++;
        cur = cur->rest_;
    }
    return length;
}

template <typename T>
/* static */ inline Result<Slist<T>*>
Slist<T>::Create(AllocationContext acx,
                 Handle<T> value,
                 Handle<Slist<T>*> rest)
{
    return acx.createSized<Slist<T>>(value, rest);
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__SLIST_HPP

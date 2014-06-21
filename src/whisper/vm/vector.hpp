#ifndef WHISPER__VM__VECTOR_HPP
#define WHISPER__VM__VECTOR_HPP


#include "vm/core.hpp"

#include <new>

namespace Whisper {
namespace VM {


// Template to annotate types which are used as parameters to
// Array<>, so that we can derive an AllocFormat for the array of
// a particular type.
template <typename T>
struct VectorTraits
{
    VectorTraits() = delete;

    // Set to true for all specializations.
    static const bool Specialized = false;

    // Give an AllocFormat for an array of type T.
    //
    // static const GC::AllocFormat VectorContentsFormat;
};

//
// A slab-allocated fixed-length array
//
template <typename T>
class VectorContents
{
    friend struct GC::TraceTraits<VectorContents<T>>;

    // T must be a field type to be usable.
    static_assert(GC::FieldTraits<T>::Specialized,
                  "Underlying type is not field-specialized.");
    static_assert(VectorTraits<T>::Specialized,
                  "Underlying type does not have a vector specialization.");

  private:
    uint32_t length_;
    HeapField<T> vals_[0];

  public:
    VectorContents(uint32_t capacity)
      : length_(0)
    {}

    VectorContents(uint32_t capacity, uint32_t len, const T *vals)
      : length_(len)
    {
        WH_ASSERT(len <= capacity);
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(this, vals[i]);
    }

    VectorContents(uint32_t capacity, uint32_t len, const T &val)
      : length_(len)
    {
        WH_ASSERT(len <= capacity);
        for (uint32_t i = 0; i < len; i++)
            vals_[i].init(this, val);
    }

    template <typename U>
    VectorContents(uint32_t capacity, const VectorContents<U> *otherContents)
      : length_(otherContents->length_)
    {
        WH_ASSERT(length_ <= capacity);
        for (uint32_t i = 0; i < length_; i++)
            vals_[i].init(this, otherContents->vals_[i]);
    }

    uint32_t length() const {
        return length_;
    }

    inline uint32_t capacity() const;

    const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    T get(uint32_t idx) const {
        return vals_[idx];
    }

    void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());
        vals_[idx].set(val, this);
    }
    void set(uint32_t idx, T &&val) {
        WH_ASSERT(idx < length());
        vals_[idx].set(val, this);
    }

    void insert(uint32_t idx, const T &val) {
        WH_ASSERT(idx <= length());
        WH_ASSERT(length() < capacity());

        uint32_t len = length();

        if (idx < len)
            shiftUp(idx);

        vals_[idx].set(val, this);
        ++length_;
    }
    void insert(uint32_t idx, T &&val) {
        WH_ASSERT(idx <= length());
        WH_ASSERT(length() < capacity());

        uint32_t len = length();

        if (idx < len)
            shiftUp(idx);

        vals_[idx].set(std::move(val), this);
        ++length_;
    }
    void append(const T &val) {
        WH_ASSERT(length() < capacity());
        vals_[length()].set(val, this);
        ++length_;
    }
    void append(T &&val) {
        WH_ASSERT(length() < capacity());
        vals_[length()].set(std::move(val), this);
        ++length_;
    }

    void erase(uint32_t idx) {
        WH_ASSERT(idx < length());

        uint32_t len = length();

        for (uint32_t i = idx; i < len - 1; i++)
            vals_[i].set(std::move(vals_[i + 1]), this);

        vals_[len - 1].destroy(this);
    }

  private:
    void shiftUp(uint32_t idx) {
        WH_ASSERT(idx < length());
        WH_ASSERT(length() < capacity());

        uint32_t len = length();
        uint32_t count = len - idx;

        vals_[len].init(std::move(vals_[len - 1]), this);
        for (uint32_t i = 1; i < count; i++)
            vals_[len - i].set(std::move(vals_[len - (i + 1)]), this);
    }
};


} // namespace VM
} // namespace Whisper

#include "vm/vector_contents_specializations.hpp"


namespace Whisper {
namespace VM {


class VectorBase
{
    friend class GC::TraceTraits<VectorBase>;

  private:
    HeapField<GC::AllocThing *> contents_;

  protected:
    VectorBase(GC::AllocThing *contents) {
        contents_.init(this, contents);
    }

    GC::AllocThing *untypedContents() const {
        return contents_;
    }

    template <typename T>
    VectorContents<T> *typedContents() const {
        return reinterpret_cast<VectorContents<T>>(untypedContents());
    }
};


template <typename T>
class Vector : public VectorBase
{
  public:
    typedef VectorContents<T> ContentsType;

  public:
    Vector(ContentsType *contents)
      : VectorBase(GC::AllocThing::From(contents))
    {}

    VectorContents<T> *contents() const {
        return this->typedContents<T>();
    }

    uint32_t length() const {
        return contents()->length();
    }

    uint32_t capacity() const {
        return contents()->capacity();
    }

    inline static Vector<T> *Create(AllocationContext &acx, uint32_t capacity);
};


} // namespace VM
} // namespace Whisper

// Include array template specializations for describing arrays to
// the GC.
#include "vm/vector_specializations.hpp"


namespace Whisper {
namespace VM {


template <typename T>
inline uint32_t
VectorContents<T>::capacity() const
{
    uint32_t size = GC::AllocThing::From(this)->size();
    size -= sizeof(VectorContents<T>);
    WH_ASSERT(size % sizeof(T) == 0);
    return size / sizeof(T);
}

template <typename T>
/* static */ inline Vector<T> *
Vector<T>::Create(AllocationContext &acx, uint32_t capacity)
{
    // Allocate contents.
    Local<VectorContents<T> *> contents(acx,
        acx.create<VectorContents<T>>(capacity));
    if (!contents)
        return nullptr;

    // Allocate vector.
    Local<Vector<T> *> vec(acx, acx.create<Vector<T>>(contents));
    if (!vec)
        return nullptr;

    return vec;
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__VECTOR_HPP

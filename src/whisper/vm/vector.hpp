#ifndef WHISPER__VM__VECTOR_HPP
#define WHISPER__VM__VECTOR_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "vm/array.hpp"

#include <new>

namespace Whisper {

namespace VM {
    template <typename T> class Vector;
    template <typename T> class VectorContents;

    // VectorTraits for a particular type T provides information about
    // T.
    template <typename T>
    struct VectorTraits {
        VectorTraits() = delete;
        static constexpr bool TRACED = true;
    };
#define UNTRACED_SPEC_(T) \
    template <> struct VectorTraits<T> {  \
        VectorTraits() = delete; \
        static constexpr bool TRACED = false; \
    }
    UNTRACED_SPEC_(bool);
    UNTRACED_SPEC_(int8_t);
    UNTRACED_SPEC_(uint8_t);
    UNTRACED_SPEC_(int16_t);
    UNTRACED_SPEC_(uint16_t);
    UNTRACED_SPEC_(int32_t);
    UNTRACED_SPEC_(uint32_t);
    UNTRACED_SPEC_(int64_t);
    UNTRACED_SPEC_(uint64_t);
    UNTRACED_SPEC_(float);
    UNTRACED_SPEC_(double);
#undef UNTRACED_SPEC_
};

// Specialize SlabThingTraits
template <typename T>
struct SlabThingTraits<VM::Vector<T>>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

template <typename T>
struct SlabThingTraits<VM::VectorContents<T>>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

// Specialize for AllocationTraits
template <typename T>
struct AllocationTraits<VM::Vector<T>>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::Vector;
    static constexpr bool TRACED = true;
};

template <typename T>
struct AllocationTraits<VM::VectorContents<T>>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::VectorContents;
    static constexpr bool TRACED = VM::VectorTraits<T>::TRACED;
};

 
namespace VM {
    
//
// The contents allocation of a vector.
//
template <typename T>
class VectorContents
{
  private:
    T vals_[0];

  public:
    static uint32_t CalculateSize(uint32_t length) {
        return sizeof(VectorContents<T>) + (length * sizeof(T));
    }

    inline VectorContents() {
    }

    static VectorContents<T> *Create(AllocationContext &cx, uint32_t length) {
        return cx.createSized<VectorContents<T>>(CalculateSize(length));
    }

    inline uint32_t length() const {
        return SlabThing::From(this)->allocSize() / sizeof(T);
    }

    inline const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    inline T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return vals_[idx];
    }

    inline const Heap<T> &get(uint32_t idx) const {
        return reinterpret_cast<const Heap<T> &>(getRaw(idx));
    }

    inline Heap<T> &get(uint32_t idx) {
        return reinterpret_cast<Heap<T> &>(getRaw(idx));
    }

    inline void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());

        // If the array is not traced, setting is simple.
        if (AllocationTraits<VectorContents<T>>::TRACED)
            get(idx).set(val, this);
        else
            getRaw(idx) = val;
    }

    template <typename... Args>
    inline void init(uint32_t idx, Args... args) {
        // If the array is not traced, setting is simple.
        if (AllocationTraits<VectorContents<T>>::TRACED)
            get(idx).init(this, args...);
        else
            new (&getRaw(idx)) T(args...);
    }

    inline void destroy(uint32_t idx) {
        // If the array is not traced, setting is simple.
        if (AllocationTraits<VectorContents<T>>::TRACED)
            get(idx).destroy(this);
        else
            getRaw(idx).~T();
    }
};


//
// A slab-allocated variable-length vector
//
template <typename T>
class Vector
{
  private:
    uint32_t length_;
    Heap<VectorContents<T> *> contents_;

  public:
    inline Vector()
      : length_(0),
        contents_(nullptr)
    {}

    bool ensureCapacity(AllocationContext &cx, uint32_t length) {
        if (capacity() >= length)
            return true;

        WH_ASSERT(length_ == 0);

        VectorContents<T> *contents = VectorContents<T>::Create(cx, length);
        if (!contents)
            return false;

        contents_->set(contents, this);
        return true;
    }

    inline uint32_t length() const {
        return length_;
    }

    inline uint32_t capacity() const {
        if (contents_ == nullptr)
            return 0;
        return contents_->length();
    }

    inline const T &get(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return contents_->getRaw(idx);
    }

    inline void set(uint32_t idx, const T &val) {
        WH_ASSERT(idx < length());
        contents_->set(idx, val);
    }

    inline void pushBackGuaranteed(const T &val) {
        WH_ASSERT(capacity() > length());
        contents_->init(length(), val);
        length_++;
    }

    inline bool pushBack(AllocationContext &cx, const T &val) {
        WH_ASSERT(capacity() >= length());
        if (capacity() == length()) {
            // Enlarge the vector.
            if (!ensureCapacity(cx, capacity() * 2))
                return false;
        }

        pushBackGuaranteed(val);
        return true;
    }

    inline void popBack() {
        WH_ASSERT(length() > 0);
        contents_->destroy(length() - 1);
        length_--;
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__VECTOR_HPP

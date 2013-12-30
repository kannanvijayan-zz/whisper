#ifndef WHISPER__VM__HASH_TABLE_HPP
#define WHISPER__VM__HASH_TABLE_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "vm/array.hpp"

#include <new>

namespace Whisper {

//
// A hashtable uses a POLICY type to determine hashing behaviour.
//
// The POLICY for a given type T have the following features:
//
//  template <typename LOOKUP>
//  const uint32_t hash(const LOOKUP &lookup);
//
//  template <typename LOOKUP>
//  const bool equal(const T &item, const LOOKUP &lookup);
//
//  static constexpr bool TRACED;
//
//  T unusedElement();
//  T deletedElement();

namespace VM {
    template <typename T, typename POLICY> class HashTable;
    template <typename T, typename POLICY> class HashTableContents;
};

// Specialize SlabThingTraits
template <typename T, typename POLICY>
struct SlabThingTraits<VM::HashTable<T, POLICY>>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

template <typename T, typename POLICY>
struct SlabThingTraits<VM::HashTableContents<T, POLICY>>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

// Specialize for AllocationTraits
template <typename T, typename POLICY>
struct AllocationTraits<VM::HashTable<T, POLICY>>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::HashTable;
    static constexpr bool TRACED = true;
};

template <typename T, typename POLICY>
struct AllocationTraits<VM::HashTableContents<T, POLICY>>
{
    static constexpr SlabAllocType ALLOC_TYPE =
                            SlabAllocType::HashTableContents;
    static constexpr bool TRACED = POLICY::TRACED;
};


namespace VM {


//
// The contents allocation of a vector.
//
template <typename T, typename POLICY>
class HashTableContents
{
    friend class HashTable<T, POLICY>;

  private:
    T elems_[0];

  public:
    static uint32_t CalculateSize(uint32_t length) {
        return sizeof(HashTableContents<T, POLICY>) + (length * sizeof(T));
    }

    inline HashTableContents(POLICY &policy) {
        uint32_t len = length();
        for (uint32_t i = 0; i < len; i++)
            new (&elems_[i]) T(policy.unusedElement());
    }

    static HashTableContents<T, POLICY> *Create(AllocationContext &cx,
                                                uint32_t length,
                                                POLICY &policy)
    {
        return cx.createSized<HashTableContents<T, POLICY>>(
                        CalculateSize(length), policy);
    }

    static constexpr bool IsTraced =
        AllocationTraits<HashTableContents<T, POLICY>>::TRACED;

    inline uint32_t length() const {
        return SlabThing::From(this)->allocSize() / sizeof(T);
    }

    inline const T &get(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return elems_[idx];
    }

    inline T &get(uint32_t idx) {
        WH_ASSERT(idx < length());
        return elems_[idx];
    }

    inline void set(uint32_t idx, const T &elem) {
        WH_ASSERT(idx < length());

        if (IsTraced)
            reinterpret_cast<Heap<T> &>(elems_[idx]).set(elem, this);
        else
            elems_[idx] = elem;
    }

    inline void destroy(uint32_t idx) {
        if (IsTraced)
            reinterpret_cast<Heap<T> &>(elems_[idx]).destroy(this);
        else
            elems_[idx].~T();
    }
};


//
// A slab-allocated variable-length vector
//
template <typename T, typename POLICY>
class HashTable
{
  public:
    class Cursor {
      private:
        uint32_t idx_;
        bool found_ : 1;
        bool valid_ : 1;

        Cursor()
          : idx_(0),
            found_(false),
            valid_(false)
        {}

        Cursor(uint32_t idx, bool found)
          : idx_(0),
            found_(found),
            valid_(true)
        {}

        Cursor(uint32_t idx)
          : idx_(0),
            found_(false),
            valid_(true)
        {}

      public:
        inline uint32_t index() const {
            return idx_;
        }

        inline bool found() const {
            return found_;
        }

        inline bool valid() const {
            return valid_;
        }

        static inline Cursor Invalid() {
            return Cursor();
        }

        static inline Cursor Found(uint32_t idx) {
            return Cursor(idx, true);
        }

        static inline Cursor NotFound(uint32_t idx) {
            return Cursor(idx);
        }
    };

    static constexpr float MAX_FILL = 0.75;
    static constexpr uint32_t START_CAPACITY = 10;

  private:
    uint32_t size_;
    Heap<HashTableContents<T, POLICY> *> contents_;
    POLICY policy_;

  public:
    inline HashTable()
      : size_(0),
        contents_(nullptr),
        policy_()
    {}

    inline HashTable(const POLICY &policy)
      : size_(0),
        contents_(nullptr),
        policy_(policy)
    {}

    inline uint32_t size() const {
        return size_;
    }

    template <typename LOOKUP>
    inline Cursor lookup(const LOOKUP &key) const {
        if (size_ == 0)
            return Cursor::Invalid();

        // Find entry.
        uint32_t hash = policy_.hash(key);
        uint32_t start = hash % capacity();
        uint32_t probe = start;

        for (;;) {
            const T &item = contents_->get(probe);
            if (policy_.equal(item, key))
                return Cursor::Found(probe);

            if (policy_.equal(item, policy_.unusedElement()))
                return Cursor::Invalid();

            probe += 1;
            probe %= capacity();
            if (probe == start)
                break;
        }

        return Cursor::Invalid();
    }

    template <typename LOOKUP>
    inline Cursor lookupForAddHelper(const LOOKUP &key) const {
        if (size_ == 0)
            return Cursor::Invalid();

        // Find entry.
        uint32_t hash = policy_.hash(key);
        uint32_t start = hash % capacity();
        uint32_t probe = start;

        bool gotDeleted = false;
        uint32_t delIdx = 0;

        for (;;) {
            const T &item = contents_->get(probe);
            if (policy_.equal(item, key))
                return Cursor::Found(probe);

            if (policy_.equal(item, policy_.unusedElement()))
                return Cursor::NotFound(probe);

            if (policy_.equal(item, policy_.deletedElement())) {
                if (!gotDeleted) {
                    gotDeleted = true;
                    delIdx = probe;
                }
            }

            probe += 1;
            probe %= capacity();
            if (probe == start)
                break;
        }

        if (gotDeleted)
            return Cursor::NotFound(delIdx);

        return Cursor::Invalid();
    }

    template <typename LOOKUP>
    inline Cursor lookupForAdd(AllocationContext *cx, const LOOKUP &lookup) {
        Cursor result = lookupForAddHelper(lookup);
        if (result.found())
            return result;

        // Else, this is a lookup for add.  Resize hash table if it's
        // too large to add another element.
        if (size_ >= capacity() * MAX_FILL) {
            if (!enlarge(cx))
                return Cursor::Invalid();

            result = lookupForAddHelper(lookup);
        }
        WH_ASSERT(result.valid() && !result.found());
        return result;
    }

    inline void add(Cursor cursor, const T &elem) {
        WH_ASSERT(cursor.valid() && !cursor.found());
        contents_->set(cursor.index(), elem);
    }

    inline void remove(Cursor cursor) {
        WH_ASSERT(cursor.valid() && cursor.found());
        contents_->set(cursor.index(), policy_.deletedElement());
    }

  private:
    inline uint32_t capacity() const {
        if (contents_ == nullptr)
            return 0;
        return contents_->length();
    }

    bool enlarge(AllocationContext &cx) {
        DebugVal<uint32_t> oldSize = size_;
        uint32_t cap = capacity();
        uint32_t newCap = cap * 2;
        if (newCap == 0)
            newCap = 10;

        HashTableContents<T, POLICY> *contents = nullptr;
        WH_ASSERT(size_ == 0);
        contents = HashTableContents<T, POLICY>::Create(cx, newCap, policy_);
        if (!contents)
            return false;
        Local<HashTableContents<T, POLICY> *> oldContents = contents_;
        contents_.set(contents, this);
        size_ = 0;

        // Re-insert elements
        for (uint32_t idx = 0; idx < cap; idx++) {
            const T &oldElem = oldContents->get(idx);
            // Skip unused and deleted elements.
            if (policy_.equal(oldElem, policy_.unusedElement()))
                continue;
            if (policy_.equal(oldElem, policy_.deletedElement()))
                continue;

            // Add element.
            Cursor curs = lookupForAddHelper(cx, oldElem);
            WH_ASSERT(curs.valid());
            WH_ASSERT(!curs.found());
            contents_->set(curs.index(), oldElem);
            size_++;
        }

        WH_ASSERT(oldSize == size_);
        return true;
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__HASH_TABLE_HPP

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
// Every hashtable is defined in terms of an entry type T.  Values
// of this type are stored in the hash table.
//
// The POLICY for hashing a type T is a class + instance satisfying
// the following:
//
//  // Given instance lookup of type |LOOKUP|, return the hash value
//  // for the instance.  Usually, LOOKUP is the "key type" of
//  // for the hash table - an isomorph to the subvalue of a value
//  // of type T that determines its hash value.
//  template <typename LOOKUP>
//  const uint32_t hash(const LOOKUP &lookup);
//
//  // Given instance lookup of type |LOOKUP|, and an instance of
//  // type |T|, return whether the two instances are equal with
//  // respect to the hash table.  This should check if |lookup|
//  // corresponds to the key contained within |item|.
//  template <typename LOOKUP>
//  bool equal(const T &item, const LOOKUP &lookup);
//
//  // If true, then added entries may introduce new pointers into
//  // the heap.
//  static constexpr bool TRACED;
//
//  // If true, then entry updates may introduce new pointers into
//  // the heap.  If UPDATES_TRACED is true, then TRACED must aslo
//  // be true.
//  static constexpr bool UPDATES_TRACED;
//
//  // Return a hash element that represents an unused element, and
//  // one that represents a deleted element.  These must not conflict
//  // with any entry that is otherwise added to the hash table.
//  T unusedElement();
//  T deletedElement();
//
//  // Given an instance of type |UPDATE|, alter an item in the hash
//  // table to contain the update.
//  template <typename UPDATE>
//  void update(T &item, const UPDATE &update);
//

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
// The contents allocation of a hash table.
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
    static constexpr bool IsUpdateTraced =
        AllocationTraits<HashTableContents<T, POLICY>>::UPDATE_TRACED;

    inline uint32_t length() const {
        return SlabThing::From(this)->allocSize() / sizeof(T);
    }

    inline const T &getRaw(uint32_t idx) const {
        WH_ASSERT(idx < length());
        return elems_[idx];
    }

    inline T &getRaw(uint32_t idx) {
        WH_ASSERT(idx < length());
        return elems_[idx];
    }

    inline const Heap<T> &get(uint32_t idx) const {
        return reinterpret_cast<const Heap<T> &>(getRaw(idx));
    }

    inline T &get(uint32_t idx) {
        return reinterpret_cast<Heap<T> &>(getRaw(idx));
    }

    inline void set(uint32_t idx, const T &elem) {
        WH_ASSERT(idx < length());
        if (IsTraced)
            get(idx).set(elem, this);
        else
            getRaw(idx) = elem;
    }

    template <typename UPDATE>
    inline void update(uint32_t idx, const UPDATE &update) {
        WH_ASSERT(idx < length());

        if (IsUpdateTraced) {
            get(idx).notifySetPre();
            POLICY::update(getRaw(idx), update);
            get(idx).notifySetPost(this);
        } else {
            POLICY::update(getRaw(idx), update);
        }
    }

    inline void destroy(uint32_t idx) {
        if (IsTraced)
            get(idx).destroy(this);
        else
            getRaw(idx).~T();
    }
};


//
// Hashtable container.
//
template <typename T, typename POLICY>
class HashTable
{
  public:
    // Cursor holds the result of lookups on the hash table.
    // An invalid cursor is returned if the lookup caused an
    // exception.  For example, if reallocating the hash table
    // failed during a lookup-for-insertion.
    //
    // If a cursor is valid, found_ indiates if an existing entry
    // with a matching record was found.
    class Cursor {
      private:
        uint32_t idx_;
        bool found_ : 1;
        bool valid_ : 1;

        Cursor(uint32_t idx, bool found, bool valid)
          : idx_(idx),
            found_(found),
            valid_(valid)
        {}

      public:
        inline uint32_t index() const {
            WH_ASSERT(valid_);
            return idx_;
        }

        inline bool found() const {
            return found_;
        }

        inline bool valid() const {
            return valid_;
        }

        static inline Cursor Invalid() {
            return Cursor(0, false, false);
        }

        static inline Cursor Found(uint32_t idx) {
            return Cursor(idx, true, true);
        }

        static inline Cursor NotFound(uint32_t idx) {
            return Cursor(idx, false, true);
        }
    };

    static constexpr float MAX_FILL = 0.75;
    static constexpr uint32_t START_CAPACITY = 10;

  protected:
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
        uint32_t cap = capacity();
        uint32_t hash = policy_.hash(key);
        uint32_t start = hash % cap;
        uint32_t probe = start;

        for (;;) {
            const T &item = contents_->getRaw(probe);
            if (policy_.equal(item, key))
                return Cursor::Found(probe);

            if (policy_.equal(item, policy_.unusedElement()))
                return Cursor::Invalid();

            probe = (probe + 1) % cap;
            if (probe == start)
                break;
        }

        return Cursor::NotFound();
    }

    template <typename LOOKUP>
    inline Cursor lookupForAddHelper(const LOOKUP &key) const {
        if (size_ == 0)
            return Cursor::Invalid();

        // Find entry.
        uint32_t cap = capacity();
        uint32_t hash = policy_.hash(key);
        uint32_t start = hash % cap;
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

            probe = (probe + 1) % cap;
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

    inline void put(Cursor cursor, const T &elem) {
        WH_ASSERT(cursor.valid() && !cursor.found());
        contents_->set(cursor.index(), elem);
        size_++;
    }

    template <typename UPDATE>
    inline void update(Cursor cursor, const UPDATE &update) {
        WH_ASSERT(cursor.valid() && cursor.found());
        contents_->update(cursor.index(), update);
    }

    inline void remove(Cursor cursor) {
        WH_ASSERT(cursor.valid() && cursor.found());
        contents_->set(cursor.index(), policy_.deletedElement());
        size_--;
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


//
// HashMap specialization of hash table.
//

//
// HashMapPair combines a key and a vaue.
//
template <typename KEY, typename VALUE>
class HashMapPair
{
  private:
    KEY key_;
    VALUE value_;

  public:
    inline HashMapPair()
      : key_(),
        value_()
    {}

    inline HashMapPair(const KEY &key, const VALUE &value)
      : key_(key),
        value_(value)
    {}

    inline const KEY &key() const {
        return key_;
    }
    inline KEY &key() {
        return key_;
    }

    inline const VALUE &value() const {
        return value_;
    }
    inline VALUE &value() {
        return value_;
    }

    template <typename UPDATE>
    inline void setValue(const UPDATE &update) {
        value_ = update;
    }
};

//
// HashMapPolicyAdapter adataps a hash map policy into a hashtable policy.
// The POLICY for hashing key/value pairs KEY and VALUE is a class + instance
// satisfying the following:
//
//  // Given instance lookup of type |LOOKUP|, return the hash value
//  // for the instance.  Usually, LOOKUP is the "key type" of
//  // for the hash table - an isomorph to the subvalue of a value
//  // of type T that determines its hash value.
//  template <typename LOOKUP>
//  const uint32_t hash(const LOOKUP &lookup);
//
//  // Given instance lookup of type |LOOKUP|, and an instance of
//  // type |KEY|, return whether the two instances are equal with
//  // respect to the hash table.  This should check if |lookup|
//  // corresponds to the key contained within |item|.
//  template <typename LOOKUP>
//  bool equal(const KEY &key, const LOOKUP &lookup);
//
//  // If true, then added entries may introduce new pointers into
//  // the heap.
//  static constexpr bool KEY_TRACED;
//
//  // If true, then entry updates may introduce new pointers into
//  // the heap.  If UPDATES_TRACED is true, then TRACED must aslo
//  // be true.
//  static constexpr bool VALUE_TRACED;
//
//  // Return a hash element that represents an unused key, and
//  // one that represents a deleted key.  These must not conflict
//  // with any entry that is otherwise added to the hash table.
//  KEY unusedKey();
//  KEY deletedKey();
//
//  // Return a default-constructed hash value.
//  VALUE emptyValue();
//

template <typename KEY, typename VALUE, typename POLICY>
class HashMapPolicyAdapter
{
    // Implements Hashtable POLICY for HashMapPair<KEY, VALUE>
    typedef HashMapPair<KEY, VALUE> Pair_;

  private:
    POLICY policy_;

  public:
    inline HashMapPolicyAdapter(const POLICY &policy) : policy_(policy) {}

    template <typename LOOKUP>
    inline const uint32_t hash(const LOOKUP &lookup) {
        return policy_.hash(lookup);
    }

    template <typename LOOKUP>
    inline bool equal(const Pair_ &item, const LOOKUP &lookup) {
        return policy_.equal(item.key(), lookup);
    }

    static constexpr bool TRACED = POLICY::KEY_TRACED || POLICY::VALUE_TRACED;
    static constexpr bool UPDATE_TRACED = POLICY::VALUE_TRACED;

    inline Pair_ unusedElement() {
        return Pair_(policy_.unusedKey(), policy_.emptyValue());
    }
    inline Pair_ deletedElement() {
        return Pair_(policy_.deletedKey(), policy_.emptyValue());
    }

    template <typename UPDATE>
    inline void update(Pair_ &item, const UPDATE &update) {
        item.value() = update;
    }
};


//
// HashMap specialization of hash table.
//
template <typename KEY, typename VALUE, typename POLICY>
class HashMap : public HashTable<HashMapPair<KEY, VALUE>,
                                 HashMapPolicyAdapter<KEY, VALUE, POLICY>>
{
    typedef HashMapPair<KEY, VALUE> Pair_;
    typedef HashMapPolicyAdapter<KEY, VALUE, POLICY> Policy_;
    typedef HashTable<Pair_, Policy_> Base_;
    typedef typename Base_::Cursor Cursor_;

  public:
    inline HashMap()
      : Base_()
    {}

    inline HashMap(const POLICY &policy)
      : Base_(Policy_(policy))
    {}

    inline bool put(AllocationContext *cx, const KEY &key, const VALUE &value)
    {
        Cursor_ curs = lookupForAdd(cx, key);
        if (!curs.valid())
            return false;
        this->put(curs, Pair_(key, value));
    }

    template <typename UPDATE>
    inline void update(const KEY &key, const UPDATE &update) {
        Cursor_ curs = lookup(key);
        WH_ASSERT(curs.valid() && curs.found());
        this->update(curs, update);
    }

    inline bool remove(const KEY &key) {
        Cursor_ curs = lookup(key);
        if (curs.found()) {
            this->remove(curs);
            return true;
        }
        return false;
    }
};


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__HASH_TABLE_HPP

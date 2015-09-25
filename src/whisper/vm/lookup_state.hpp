#ifndef WHISPER__VM__LOOKUP_STATE_HPP
#define WHISPER__VM__LOOKUP_STATE_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/properties.hpp"
#include "vm/array.hpp"
#include "vm/wobject.hpp"

/**
 * A LookupState object keeps track of the current 'state' of
 * a property access lookup (either for a property get or set).
 */

namespace Whisper {
namespace VM {


class LookupSeenObjects
{
    friend class TraceTraits<LookupSeenObjects>;
  public:
    static inline Wobject* SENTINEL() {
        return reinterpret_cast<Wobject*>(0x1u);
    }

  private:
    uint32_t size_;
    uint32_t filled_;
    HeapField<Wobject*> seen_[0];

    static constexpr float MaxFillRatio = 0.75;

  public:
    LookupSeenObjects(uint32_t size)
      : size_(size),
        filled_(0)
    {
        for (uint32_t i = 0; i < size_; i++)
            seen_[i].init(nullptr, this);
    }

    static uint32_t CalculateSize(uint32_t size) {
        return sizeof(LookupSeenObjects) + (sizeof(Wobject*) * size);
    }

    static Result<LookupSeenObjects*> Create(
        AllocationContext acx, uint32_t size);

    static Result<LookupSeenObjects*> Create(
        AllocationContext acx,
        uint32_t size,
        Handle<LookupSeenObjects*> other);

    uint32_t size() const {
        return size_;
    }
    uint32_t filled() const {
        return filled_;
    }
    bool contains(Wobject* obj) const;

    bool canAdd() const {
        return filled_ < (size_ * MaxFillRatio);
    }
    void add(Wobject* obj);

    bool indexHasValue(uint32_t index) const {
        WH_ASSERT(index < size());
        Wobject* ptr = seen_[index].get();
        return (ptr != nullptr) && (ptr != SENTINEL());
    }
  private:
    void rehashIndex(uint32_t index);
};


class LookupNode
{
    friend class TraceTraits<LookupNode>;
  private:
    HeapField<LookupNode*> parent_;
    HeapField<Wobject*> object_;
    HeapField<Array<Wobject*>*> delegates_;
    uint32_t index_;

  public:
    LookupNode(Handle<Wobject*> object)
      : parent_(nullptr),
        object_(object),
        delegates_(nullptr),
        index_(0)
    {
        WH_ASSERT(object.get() != nullptr);
    }
    LookupNode(Handle<LookupNode*> parent,
               Handle<Wobject*> object)
      : parent_(parent),
        object_(object),
        delegates_(nullptr),
        index_(0)
    {
        WH_ASSERT(parent.get() != nullptr);
        WH_ASSERT(object.get() != nullptr);
    }

    static Result<LookupNode*> Create(AllocationContext acx,
                                      Handle<Wobject*> object);

    static Result<LookupNode*> Create(AllocationContext acx,
                                      Handle<LookupNode*> parent,
                                      Handle<Wobject*> object);

    LookupNode* parent() const {
        return parent_;
    }
    Wobject* object() const {
        return object_;
    }
    Array<Wobject*>* delegates() const {
        return delegates_;
    }
    void setDelegates(Array<Wobject*>* delgs) {
        delegates_.set(delgs, this);
    }
    uint32_t index() const {
        return index_;
    }
    void setIndex(uint32_t index) {
        WH_ASSERT(delegates_.get() != nullptr);
        WH_ASSERT(index <= delegates_->length());
        index_ = index;
    }
};


class LookupState
{
    friend class TraceTraits<LookupState>;
  private:
    HeapField<Wobject*> receiver_;
    HeapField<String*> name_;
    HeapField<LookupSeenObjects*> seen_;
    HeapField<LookupNode*> node_;

  public:
    LookupState(Handle<Wobject*> receiver,
                Handle<String*> name,
                Handle<LookupSeenObjects*> seen,
                Handle<LookupNode*> node)
      : receiver_(receiver),
        name_(name),
        seen_(seen),
        node_(node)
    {
        WH_ASSERT(receiver != nullptr);
        WH_ASSERT(name != nullptr);
        WH_ASSERT(seen != nullptr);
        WH_ASSERT(node != nullptr);
    }

    static Result<LookupState*> Create(AllocationContext acx,
                                       Handle<Wobject*> receiver,
                                       Handle<String*> name);

    Wobject* receiver() const {
        return receiver_;
    }
    String* name() const {
        return name_;
    }
    LookupSeenObjects* seen() const {
        return seen_;
    }
    LookupNode* node() const {
        return node_;
    }

    static OkResult NextNode(AllocationContext acx,
                             Handle<LookupState*> lookupState,
                             MutHandle<LookupNode*> nodeOut);

    static OkResult LinkNextNode(AllocationContext acx,
                                 Handle<LookupState*> lookupState,
                                 Handle<LookupNode*> parent,
                                 uint32_t index,
                                 MutHandle<LookupNode*> nodeOut);

  private:
    bool wasSeen(Wobject* obj) const {
        return seen_->contains(obj);
    }

    static OkResult AddToSeen(AllocationContext acx,
                              Handle<LookupState*> lookupState,
                              Handle<Wobject*> obj);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::LookupSeenObjects>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    inline static void Scan(Scanner& scanner,
                            VM::LookupSeenObjects const& lookupSeenObjects,
                            void const* start, void const* end)
    {
        for (uint32_t i = 0; i < lookupSeenObjects.size_; i++) {
            if (!lookupSeenObjects.indexHasValue(i))
                continue;
            lookupSeenObjects.seen_[i].scan(scanner, start, end);
        }
    }

    template <typename Updater>
    inline static void Update(Updater& updater,
                              VM::LookupSeenObjects& lookupSeenObjects,
                              void const* start, void const* end)
    {
        for (uint32_t i = 0; i < lookupSeenObjects.size_; i++) {
            if (!lookupSeenObjects.indexHasValue(i))
                continue;
            // Save old value before checking.
            VM::Wobject* oldValue = lookupSeenObjects.seen_[i].get();
            lookupSeenObjects.seen_[i].update(updater, start, end);
            VM::Wobject* newValue = lookupSeenObjects.seen_[i].get();

            // If value changed, rehash it.
            if (newValue != oldValue)
                lookupSeenObjects.rehashIndex(i);
        }
    }
};

template <>
struct TraceTraits<VM::LookupNode>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    inline static void Scan(Scanner& scanner,
                            VM::LookupNode const& lookupNode,
                            void const* start, void const* end)
    {
        lookupNode.parent_.scan(scanner, start, end);
        lookupNode.object_.scan(scanner, start, end);
        lookupNode.delegates_.scan(scanner, start, end);
    }

    template <typename Updater>
    inline static void Update(Updater& updater,
                              VM::LookupNode& lookupNode,
                              void const* start, void const* end)
    {
        lookupNode.parent_.update(updater, start, end);
        lookupNode.object_.update(updater, start, end);
        lookupNode.delegates_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::LookupState>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    inline static void Scan(Scanner& scanner,
                            VM::LookupState const& lookupState,
                            void const* start, void const* end)
    {
        lookupState.receiver_.scan(scanner, start, end);
        lookupState.name_.scan(scanner, start, end);
        lookupState.seen_.scan(scanner, start, end);
        lookupState.node_.scan(scanner, start, end);
    }

    template <typename Updater>
    inline static void Update(Updater& updater,
                              VM::LookupState& lookupState,
                              void const* start, void const* end)
    {
        lookupState.receiver_.update(updater, start, end);
        lookupState.name_.update(updater, start, end);
        lookupState.seen_.update(updater, start, end);
        lookupState.node_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__LOOKUP_STATE_HPP

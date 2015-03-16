#ifndef WHISPER__VM__LOOKUP_STATE_HPP
#define WHISPER__VM__LOOKUP_STATE_HPP

#include "vm/core.hpp"
#include "vm/properties.hpp"
#include "vm/array.hpp"
#include "vm/wobject.hpp"

/**
 * A LookupState object keeps track of the current 'state' of
 * a property access lookup (either for a property get or set).
 */

namespace Whisper {
namespace VM {

class LookupNode;
class LookupState;

} // namespace VM


template <>
struct HeapTraits<VM::LookupNode>
{
    HeapTraits() = delete;
    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = HeapFormat::LookupNode;
    static constexpr bool VarSized = false;
};


template <>
struct HeapTraits<VM::LookupState>
{
    HeapTraits() = delete;
    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = HeapFormat::LookupState;
    static constexpr bool VarSized = false;
};


namespace VM {


class LookupNode
{
    friend class TraceTraits<LookupNode>;
  private:
    HeapField<LookupNode *> parent_;
    HeapField<Wobject *> object_;
    HeapField<Array<Wobject *> *> delegates_;
    uint32_t index_;

  public:
    LookupNode(LookupNode *parent,
               Wobject *object,
               Array<Wobject *> *delegates,
               uint32_t index)
      : parent_(parent),
        object_(object),
        delegates_(delegates),
        index_(index)
    {
        WH_ASSERT(parent != nullptr);
        WH_ASSERT(object != nullptr);
        WH_ASSERT(delegates != nullptr);
    }

    LookupNode *Create(AllocationContext acx,
                       Handle<LookupNode *> parent,
                       Handle<Wobject *> object,
                       Handle<Array<Wobject *> *> delegates,
                       uint32_t index);

    LookupNode *parent() const {
        return parent_;
    }
    Wobject *object() const {
        return object_;
    }
    Array<Wobject *> *delegates() const {
        return delegates_;
    }
    void setDelegates(Array<Wobject *> *delgs) {
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
    HeapField<Wobject *> receiver_;
    HeapField<String *> name_;
    HeapField<LookupNode *> node_;

  public:
    LookupState(Wobject *receiver, String *name, LookupNode *node)
      : receiver_(receiver),
        name_(name),
        node_(node)
    {
        WH_ASSERT(receiver != nullptr);
        WH_ASSERT(name != nullptr);
        WH_ASSERT(node != nullptr);
    }

    LookupState *Create(AllocationContext acx,
                        Handle<Wobject *> receiver,
                        Handle<String *> name,
                        Handle<LookupNode *> node);

    Wobject *receiver() const {
        return receiver_;
    }
    String *name() const {
        return name_;
    }
    LookupNode *node() const {
        return node_;
    }
};


} // namespace VM


//
// GC Specializations
//


template <>
struct HeapFormatTraits<HeapFormat::LookupNode>
{
    HeapFormatTraits() = delete;
    static constexpr bool Specialized = true;
    typedef VM::LookupNode Type;
};
template <>
struct TraceTraits<VM::LookupNode>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    inline static void Scan(Scanner &scanner,
                            const VM::LookupNode &lookupNode,
                            const void *start, const void *end)
    {
        lookupNode.parent_.scan(scanner, start, end);
        lookupNode.object_.scan(scanner, start, end);
        lookupNode.delegates_.scan(scanner, start, end);
    }

    template <typename Updater>
    inline static void Update(Updater &updater,
                              VM::LookupNode &lookupNode,
                              const void *start, const void *end)
    {
        lookupNode.parent_.update(updater, start, end);
        lookupNode.object_.update(updater, start, end);
        lookupNode.delegates_.update(updater, start, end);
    }
};


template <>
struct HeapFormatTraits<HeapFormat::LookupState>
{
    HeapFormatTraits() = delete;
    static constexpr bool Specialized = true;
    typedef VM::LookupState Type;
};
template <>
struct TraceTraits<VM::LookupState>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    inline static void Scan(Scanner &scanner,
                            const VM::LookupState &lookupState,
                            const void *start, const void *end)
    {
        lookupState.receiver_.scan(scanner, start, end);
        lookupState.name_.scan(scanner, start, end);
        lookupState.node_.scan(scanner, start, end);
    }

    template <typename Updater>
    inline static void Update(Updater &updater,
                              VM::LookupState &lookupState,
                              const void *start, const void *end)
    {
        lookupState.receiver_.update(updater, start, end);
        lookupState.name_.update(updater, start, end);
        lookupState.node_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__LOOKUP_STATE_HPP

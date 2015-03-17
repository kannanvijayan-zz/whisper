
#include <functional>

#include "runtime_inlines.hpp"
#include "vm/lookup_state.hpp"

namespace Whisper {
namespace VM {


/* static */ LookupSeenObjects *
LookupSeenObjects::Create(AllocationContext acx, uint32_t size)
{
    return acx.createSized<LookupSeenObjects>(CalculateSize(size), size);
}

/* static */ LookupSeenObjects *
LookupSeenObjects::Create(AllocationContext acx, uint32_t size,
                          Handle<LookupSeenObjects *> other)
{
    // The given size should be enough to fit the other's pointers.
    WH_ASSERT(other->filled_ < (size * MaxFillRatio));

    Local<LookupSeenObjects *> newSeen(acx, Create(acx, size));
    if (!newSeen.get())
        return nullptr;

    // Add old entries to new.
    for (uint32_t i = 0; i < other->size_; i++) {
        Wobject *obj = other->seen_[i].get();
        if (obj != nullptr) {
            WH_ASSERT(newSeen->canAdd());
            WH_ASSERT(!newSeen->contains(obj));
            newSeen->add(obj);
        }
    }

    return newSeen.get();
}

bool
LookupSeenObjects::contains(Wobject *obj) const
{
    // Hash the object.
    std::hash<Wobject *> hasher;
    size_t index = hasher(obj) % size_;

    // Check partial hash.
    size_t probe = index;
    for (;;) {
        Wobject *entry = seen_[probe].get();
        if (entry == nullptr)
            return false;

        if (entry == obj)
            return true;

        probe += 1;
        probe %= size_;
        if (probe == index)
            return false;
    }
}

void
LookupSeenObjects::add(Wobject *obj)
{
    WH_ASSERT(!contains(obj));
    WH_ASSERT(canAdd());

    // Hash the object.
    std::hash<Wobject *> hasher;
    size_t index = hasher(obj) % size_;

    // Check partial hash.
    size_t probe = index;
    for (;;) {
        Wobject *entry = seen_[probe].get();
        if (entry == nullptr) {
            seen_[probe].set(obj, this);
            return;
        }
            
        probe += 1;
        probe %= size_;
        WH_ASSERT(probe != index);
    }
}

/* static */ LookupNode *
LookupNode::Create(AllocationContext acx,
                   Handle<Wobject *> object)
{
    return acx.create<LookupNode>(object);
}

/* static */ LookupNode *
LookupNode::Create(AllocationContext acx,
                   Handle<LookupNode *> parent,
                   Handle<Wobject *> object)
{
    return acx.create<LookupNode>(parent, object);
}

/* static */ LookupState *
LookupState::Create(AllocationContext acx,
                    Handle<Wobject *> receiver,
                    Handle<String *> name)
{
    Local<LookupSeenObjects *> seen(acx, LookupSeenObjects::Create(acx, 10));
    if (!seen.get())
        return nullptr;

    Local<LookupNode *> node(acx, LookupNode::Create(acx, receiver));
    if (!node.get())
        return nullptr;

    return acx.create<LookupState>(receiver, name,
                                   seen.handle(), node.handle());
}

/* static */ bool
LookupState::NextNode(AllocationContext acx,
                      Handle<LookupState *> lookupState,
                      MutHandle<LookupNode *> nodeOut)
{
    ThreadContext *cx = acx.threadContext();

    // node_ refers to a leaf-level node whose |delegates| field is null.
    Local<LookupNode *> cur(acx, lookupState->node_);
    WH_ASSERT(cur->delegates() == nullptr);

    // Check if current object has any unseen delegates.
    Local<Wobject *> obj(acx, cur->object());
    Local<Array<Wobject *> *> delgs(acx, nullptr);
    if (!Wobject::GetDelegates(cx, obj, delgs.mutHandle()))
        return false;

    if (delgs->length() > 0) {
        // This object has delegates.  Find the first unseen delegate.
        for (uint32_t i = 0; i < delgs->length(); i++) {
            if (lookupState->wasSeen(delgs->get(i)))
                continue;
            cur->setDelegates(delgs);
            return LinkNextNode(acx, lookupState, cur, i, nodeOut);
        }
        // If we got here, it means that all the delegates were seen,
        // or the objet had no delegates.
    }

    // Walk up the chain until we find a next-delegate that
    // has not been seen.
    for (;;) {
        // Move to parent.
        cur = cur->parent();
        if (cur.get() == nullptr) {
            nodeOut = nullptr;
            return true;
        }

        // Search on from index.
        for (uint32_t i = cur->index(); i < cur->delegates()->length(); i++) {
            if (lookupState->wasSeen(cur->delegates()->get(i)))
                continue;
            return LinkNextNode(acx, lookupState, cur, i, nodeOut);
        }
    }

    // Walk up chain ended.
    lookupState->node_.set(nullptr, lookupState.get());
    nodeOut.set(nullptr);
    return true;
}

bool
LookupState::LinkNextNode(AllocationContext acx,
                          Handle<LookupState *> lookupState,
                          Handle<LookupNode *> parent,
                          uint32_t index,
                          MutHandle<LookupNode *> nodeOut)
{
    Local<Wobject *> obj(acx, parent->delegates()->get(index));

    Local<LookupNode *> newNode(acx, LookupNode::Create(acx, parent, obj));
    if (!newNode.get())
        return false;

    if (!AddToSeen(acx, lookupState, obj))
        return false;

    parent->setIndex(index);

    lookupState->node_.set(newNode, lookupState.get());
    nodeOut = newNode.get();
    return true;
}

bool
LookupState::AddToSeen(AllocationContext acx,
                       Handle<LookupState *> lookupState,
                       Handle<Wobject *> obj)
{
    WH_ASSERT(!lookupState->seen_->contains(obj));
    if (lookupState->seen_->canAdd()) {
        lookupState->seen_->add(obj);
        return true;
    }
    Local<LookupSeenObjects *> oldSeen(acx, lookupState->seen_);

    // Replace seen_ with a new larger-sized set.
    Local<LookupSeenObjects *> newSeen(acx,
        LookupSeenObjects::Create(acx, oldSeen->size() * 2, oldSeen));
    if (!newSeen.get())
        return false;

    WH_ASSERT(newSeen->canAdd());
    lookupState->seen_.set(newSeen, lookupState.get());

    lookupState->seen_->add(obj);
    return true;
}


} // namespace VM
} // namespace Whisper

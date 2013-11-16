
#include <algorithm>

#include "value_inlines.hpp"
#include "rooting_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/string.hpp"
#include "vm/object.hpp"

namespace Whisper {
namespace VM {


HashObject::HashObject(Handle<Object *> prototype)
  : prototype_(prototype), entries_(0), mappings_(nullptr)
{}

bool
HashObject::initialize(RunContext *cx)
{
    WH_ASSERT(!mappings_);

    Tuple *tup;
    if (!cx->inHatchery().createTuple(INITIAL_ENTRIES * 2, tup))
        return false;

    mappings_.set(tup, this);
    if (!mappings_)
        return false;

    return true;
}

Handle<Object *>
HashObject::prototype() const
{
    return prototype_;
}

Handle<Tuple *>
HashObject::mappings() const
{
    return mappings_;
}

uint32_t
HashObject::propertyCapacity() const
{
    WH_ASSERT(mappings_->size() % 2 == 0);
    return mappings_->size() / 2;
}

uint32_t
HashObject::numProperties() const
{
    return entries_;
}

void
HashObject::setPrototype(Handle<Object *> newProto)
{
    prototype_.set(newProto, this);
}

bool
HashObject::setOwnProperty(RunContext *cx, Handle<Value> keyString,
                           Handle<Value> val)
{
    uint32_t entry = lookupOwnProperty(cx, keyString, /*forAdd=*/true);
    WH_ASSERT(entry != UINT32_MAX);

    Handle<Value> entryKey = getEntryKey(entry);

    if (entryKey->isString()) {
        mappings_->set(ValueSlotOffset(entry), val);
        return true;
    }

    WH_ASSERT(getEntryKey(entry)->isUndefined() ||
              getEntryKey(entry)->isFalse());

    // Key needs to be interned before being used as a property name,
    // but only if it's not an indexed string.
    Root<Value> keyval(cx);
    int32_t id;
    if (IsInt32IdString(keyString, &id)) {
        keyval = keyString;
    } else {
        Root<LinearString *> internedKey(cx);
        if (!cx->stringTable().addString(keyString, &internedKey))
            return false;
        keyval = Value::HeapString(internedKey);
    }

    // Otherwise, number of entries is about to grow.  Check to see
    // if mapping should be enlarged.
    if (entries_ >= propertyCapacity() * MAX_FILL_RATIO) {
        if (!enlarge(cx))
            return false;

        entry = lookupOwnProperty(cx, keyString, /*forAdd=*/true);
        WH_ASSERT(entry != UINT32_MAX);

        // All deleted entries will have been reset during the enlarge.
        WH_ASSERT(getEntryKey(entry)->isUndefined());
    }

    setEntryKey(entry, keyval);
    setEntryValue(entry, val);
    return true;
}

uint32_t
HashObject::lookupOwnProperty(RunContext *cx, Handle<Value> keyString,
                              bool forAdd)
{
    Root<Value> key(cx);

    // Normalize key value.
    int32_t id;
    if (IsInt32IdString(keyString, &id)) {
        key = Value::ImmIndexString(id);
    } else {
        LinearString *linstr = cx->stringTable().lookupString(keyString);
        if (!linstr)
            return false;
        WH_ASSERT(linstr->isInterned());
        key = Value::HeapString(linstr);
    }

    uint32_t entryCount = propertyCapacity();
    uint32_t addEntry = UINT32_MAX;
    uint32_t hash = hashValue(cx, keyString);

    WH_ASSERT(mappings_);
    for (uint32_t i = 0; i < entryCount; i++) {
        uint32_t entry = (hash + i) % entryCount;
        Handle<Value> entryKey = getEntryKey(entry);

        if (entryKey->isUndefined()) {
            if (forAdd && addEntry < UINT32_MAX)
                return addEntry;
            return entry;
        }

        if (entryKey == key)
            return entry;

        WH_ASSERT(entryKey->isFalse() || entryKey->isImmIndexString() ||
                  (entryKey->isHeapString() &&
                   entryKey->heapStringPtr()->isLinearString() &&
                   entryKey->heapStringPtr()->toLinearString()->isInterned()));

        if (forAdd && addEntry == UINT32_MAX && entryKey->isFalse())
            addEntry = entry;
    }

    WH_UNREACHABLE("Completely full HashObject table should not ever happen!");
    return UINT32_MAX;
}

uint32_t
HashObject::hashValue(RunContext *cx, const Value &key) const
{
    WH_ASSERT(key.isImmIndexString() ||
              (key.isHeapString() && key.heapStringPtr()->isLinearString() &&
               key.heapStringPtr()->toLinearString()->isInterned()));

    if (key.isImmIndexString()) {
        int32_t idx = key.immIndexStringValue();
        return idx ^ cx->threadContext()->spoiler();
    }

    return FNVHashString(cx->threadContext()->spoiler(), key.heapStringPtr());
}

/*static*/ uint32_t
HashObject::KeySlotOffset(uint32_t entry)
{
    return entry * 2;
}

/*static*/ uint32_t
HashObject::ValueSlotOffset(uint32_t entry)
{
    return (entry * 2) + 1;
}


Handle<Value>
HashObject::getEntryKey(uint32_t entry) const
{
    return mappings_->get(KeySlotOffset(entry));
}

Handle<Value>
HashObject::getEntryValue(uint32_t entry) const
{
    return mappings_->get(ValueSlotOffset(entry));
}

void
HashObject::setEntryKey(uint32_t entry, const Value &key)
{
    mappings_->set(KeySlotOffset(entry), key);
}

void
HashObject::setEntryValue(uint32_t entry, const Value &val)
{
    mappings_->set(ValueSlotOffset(entry), val);
}

bool
HashObject::enlarge(RunContext *cx)
{
    Root<Tuple *> oldMappings(cx, mappings_);
    uint32_t curSize = propertyCapacity();
    
    // Allocate a new mapping with double capacity.
    Root<Tuple *> newMappings(cx);
    if (!cx->inHatchery().createTuple(curSize * 2 * 2, newMappings))
        return false;
    mappings_.set(newMappings, this);

    // Add old entries to table.
    for (uint32_t i = 0; i < curSize; i++) {
        Handle<Value> oldKey = oldMappings->get(KeySlotOffset(i));
        Handle<Value> oldVal = oldMappings->get(ValueSlotOffset(i));

        WH_ASSERT(oldKey->isUndefined() || oldKey->isFalse() ||
                  oldKey->isString());
        if (!oldKey->isString())
            continue;

        uint32_t entry = lookupOwnProperty(cx, oldKey, /*forAdd=*/true);
        WH_ASSERT(entry != UINT32_MAX);

        WH_ASSERT(getEntryKey(entry)->isUndefined());

        setEntryKey(entry, oldKey);
        setEntryValue(entry, oldVal);
    }

    return true;
}



} // namespace VM
} // namespace Whisper


#include "rooting_inlines.hpp"
#include "runtime_inlines.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/string_table.hpp"

namespace Whisper {
namespace VM {


//
// StringTable
//

StringTable::Query::Query(uint32_t length, const uint8_t *data)
  : data(data), length(length), isEightBit(true)
{}

StringTable::Query::Query(uint32_t length, const uint16_t *data)
  : data(data), length(length), isEightBit(false)
{}

const uint8_t *
StringTable::Query::eightBitData() const
{
    WH_ASSERT(isEightBit);
    return reinterpret_cast<const uint8_t *>(data);
}

const uint16_t *
StringTable::Query::sixteenBitData() const
{
    WH_ASSERT(!isEightBit);
    return reinterpret_cast<const uint16_t *>(data);
}

StringTable::StringOrQuery::StringOrQuery(const HeapString *str)
  : ptr(reinterpret_cast<uintptr_t>(str))
{}

StringTable::StringOrQuery::StringOrQuery(const Query *str)
  : ptr(reinterpret_cast<uintptr_t>(str) | 1)
{}

bool
StringTable::StringOrQuery::isHeapString() const
{
    return !(ptr & 1);
}

bool
StringTable::StringOrQuery::isQuery() const
{
    return ptr & 1;
}

const HeapString *
StringTable::StringOrQuery::toHeapString() const
{
    WH_ASSERT(isHeapString());
    return reinterpret_cast<const HeapString *>(ptr);
}

const StringTable::Query *
StringTable::StringOrQuery::toQuery() const
{
    WH_ASSERT(isQuery());
    return reinterpret_cast<const Query *>(ptr);
}

StringTable::StringTable(uint32_t spoiler)
  : spoiler_(spoiler),
    entries_(0),
    tuple_(nullptr)
{}


bool
StringTable::initialize(RunContext *cx)
{
    WH_ASSERT(tuple_ == nullptr);

    // Allocate a new tuple with reasonable capacity in tenured space.
    if (!cx->inTenured().createTuple(INITIAL_TUPLE_SIZE, tuple_))
        return false;

    return true;
}

LinearString *
StringTable::lookupString(RunContext *cx, const HeapString *str)
{
    LinearString *result;
    lookupSlot(cx, StringOrQuery(str), &result);
    return result;
}

LinearString *
StringTable::lookupString(RunContext *cx, uint32_t length, const uint8_t *str)
{
    Query q(length, str);
    LinearString *result;
    lookupSlot(cx, StringOrQuery(&q), &result);
    return result;
}

LinearString *
StringTable::lookupString(RunContext *cx, uint32_t length, const uint16_t *str)
{
    Query q(length, str);
    LinearString *result;
    lookupSlot(cx, StringOrQuery(&q), &result);
    return result;
}

bool
StringTable::addString(RunContext *cx, Handle<HeapString *> string,
                       MutHandle<LinearString *> interned)
{
    // Check if |string| is already a LinearString and marked as interned.
    if (string->isLinearString() && string->toLinearString()->isInterned()) {
        interned = string->toLinearString();
        return true;
    }

    // Check for existing interned string in table.
    uint32_t slot = lookupSlot(cx, StringOrQuery(string), &interned.get());
    if (interned)
        return true;

    // Allocate tenured LinearString copy (marked interned).
    uint32_t size = string->length() * 2;
    interned = cx->inTenured().createSized<LinearString>(
                            size, string, /*interned=*/true,
                            /*group=*/LinearString::Group::Unknown);
    if (!interned)
        return false;

    // Resize table if necessary.
    if (entries_ >= tuple_->size() * MAX_FILL_RATIO) {
        if (!enlarge(cx))
            return false;
        slot = lookupSlot(cx, StringOrQuery(string), &interned.get());
        WH_ASSERT(!interned);
    }

    // Store interned string.
    tuple_->set(slot, Value::HeapString(interned.get()));
    return true;
}


uint32_t
StringTable::lookupSlot(RunContext *cx, const StringOrQuery &str,
                        LinearString **result)
{
    uint32_t hash = hashString(str);
    uint32_t slotCount = tuple_->size();

    *result = nullptr;

    WH_ASSERT(tuple_);
    for (uint32_t i = 0; i < slotCount; i++) {
        uint32_t slot = (hash + i) % slotCount;
        Handle<Value> slotVal = tuple_->get(slot);
        if (slotVal->isUndefined())
            return slot;

        if (slotVal->isHeapString()) {
            HeapString *heapStr = slotVal->heapStringPtr();
            WH_ASSERT(heapStr->isLinearString());
            LinearString *linearStr = heapStr->toLinearString();

            if (compareStrings(linearStr, str) == 0) {
                *result = linearStr;
                return slot;
            }
        }

        // Only other option is deleted slot.
        WH_ASSERT(slotVal->isFalse());
    }

    WH_UNREACHABLE("Completely full StringTable should not ever happen!");
    return UINT32_MAX;
}

uint32_t
StringTable::hashString(const StringOrQuery &str)
{
    if (str.isQuery()) {
        const Query *query = str.toQuery();
        if (query->isEightBit) {
            return FNVHashString(spoiler_, query->length,
                                 query->eightBitData());
        }

        return FNVHashString(spoiler_, query->length, query->sixteenBitData());
    }

    WH_ASSERT(str.isHeapString());
    const HeapString *heapStr = str.toHeapString();

    if (heapStr->isLinearString()) {
        const LinearString *linStr = heapStr->toLinearString();
        return FNVHashString(spoiler_, linStr->length(), linStr->data());
    }

    return FNVHashString(spoiler_, heapStr);
}

int
StringTable::compareStrings(LinearString *a,
                            const StringOrQuery &b)
{
    if (b.isQuery()) {
        const Query *query = b.toQuery();
        if (query->isEightBit) {
            return CompareStrings(a->length(), a->data(),
                                  query->length, query->eightBitData());
        }
        return CompareStrings(a->length(), a->data(),
                              query->length, query->sixteenBitData());
    }

    WH_ASSERT(b.isHeapString());
    const HeapString *heapStr = b.toHeapString();

    if (heapStr->isLinearString()) {
        const LinearString *linStr = heapStr->toLinearString();
        return CompareStrings(a->length(), a->data(),
                              linStr->length(), linStr->data());
    }
    return CompareStrings(a->length(), a->data(), heapStr);
}

bool
StringTable::enlarge(RunContext *cx)
{
    Root<Tuple *> oldTuple(cx, tuple_);
    uint32_t curSize = tuple_->size();
    
    // Allocate a new tuple with double capacity.
    if (!cx->inTenured().createTuple(curSize * 2, tuple_))
        return false;

    // Add old strings to table.
    for (uint32_t i = 0; i < curSize; i++) {
        Handle<Value> oldVal = tuple_->get(i);
        WH_ASSERT(oldVal->isUndefined() || oldVal->isFalse() ||
                  oldVal->isHeapString());
        if (!oldVal->isHeapString())
            continue;

        WH_ASSERT(oldVal->heapStringPtr()->isLinearString());
        LinearString *oldStr = oldVal->heapStringPtr()->toLinearString();
        

        // Check for existing interned string in table.
        LinearString *dummy;
        uint32_t slot = lookupSlot(cx, StringOrQuery(oldStr), &dummy);
        WH_ASSERT(dummy == nullptr);

        tuple_->set(slot, Value::HeapString(oldStr));
    }

    return true;
}


} // namespace VM
} // namespace Whisper
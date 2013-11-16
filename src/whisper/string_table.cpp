
#include "rooting_inlines.hpp"
#include "runtime_inlines.hpp"
#include "value_inlines.hpp"
#include "runtime.hpp"
#include "string_table.hpp"
#include "vm/heap_thing_inlines.hpp"
#include "vm/string.hpp"
#include "vm/tuple.hpp"

namespace Whisper {


//
// StringTable
//

StringTable::Query::Query(const uint8_t *data, uint32_t length)
  : data(data), length(length), isEightBit(true)
{}

StringTable::Query::Query(const uint16_t *data, uint32_t length)
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

StringTable::StringOrQuery::StringOrQuery(const VM::HeapString *str)
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

const VM::HeapString *
StringTable::StringOrQuery::toHeapString() const
{
    WH_ASSERT(isHeapString());
    return reinterpret_cast<const VM::HeapString *>(ptr);
}

const StringTable::Query *
StringTable::StringOrQuery::toQuery() const
{
    WH_ASSERT(isQuery());
    return reinterpret_cast<const Query *>(ptr);
}

StringTable::StringTable()
  : cx_(nullptr),
    entries_(0),
    tuple_(nullptr)
{}


bool
StringTable::initialize(ThreadContext *cx)
{
    WH_ASSERT(cx_ == nullptr);
    WH_ASSERT(tuple_ == nullptr);

    cx_ = cx;

    // Allocate a new tuple with reasonable capacity in tenured space.
    if (!cx->inTenured().createTuple(INITIAL_TUPLE_SIZE, tuple_))
        return false;

    return true;
}

VM::LinearString *
StringTable::lookupString(const Value &strval)
{
    WH_ASSERT(strval.isString());
    WH_ASSERT(!VM::IsInt32IdString(strval));

    if (strval.isImmString()) {
        uint16_t buf[Value::ImmStringMaxLength];
        uint32_t len = strval.readImmString(buf);
        return lookupString(buf, len);
    }

    WH_ASSERT(strval.isHeapString());
    return lookupString(strval.heapStringPtr());
}

VM::LinearString *
StringTable::lookupString(VM::HeapString *str)
{
    if (str->isLinearString()) {
        VM::LinearString *linStr = str->toLinearString();
        if (linStr->isInterned())
            return linStr;

        return lookupString(linStr->data(), linStr->length());
    }

    VM::LinearString *result;
    lookupSlot(StringOrQuery(str), &result);
    return result;
}

VM::LinearString *
StringTable::lookupString(const uint8_t *str, uint32_t length)
{
    Query q(str, length);
    VM::LinearString *result;
    lookupSlot(StringOrQuery(&q), &result);
    return result;
}

VM::LinearString *
StringTable::lookupString(const uint16_t *str, uint32_t length)
{
    Query q(str, length);
    VM::LinearString *result;
    lookupSlot(StringOrQuery(&q), &result);
    return result;
}

bool
StringTable::addString(const uint8_t *str, uint32_t length,
                       MutHandle<VM::LinearString *> result)
{
    WH_ASSERT(!VM::IsInt32IdString(str, length));

    // Check for existing interned string in table.
    Query q(str, length);
    uint32_t slot = lookupSlot(StringOrQuery(&q), &result.get());
    if (result)
        return true;

    // Allocate tenured LinearString copy (marked interned).
    result = cx_->inTenured().createSized<VM::LinearString>(
                            length * 2, str, /*interned=*/true);
    if (!result)
        return false;

    return insertString(result, slot);
}

bool
StringTable::addString(const uint16_t *str, uint32_t length,
                       MutHandle<VM::LinearString *> result)
{
    WH_ASSERT(!VM::IsInt32IdString(str, length));

    // Check for existing interned string in table.
    Query q(str, length);
    uint32_t slot = lookupSlot(StringOrQuery(&q), &result.get());
    if (result)
        return true;

    // Allocate tenured LinearString copy (marked interned).
    result = cx_->inTenured().createSized<VM::LinearString>(
                            length * 2, str, /*interned=*/true);
    if (!result)
        return false;

    return insertString(result, slot);
}

bool
StringTable::addString(Handle<VM::HeapString *> string,
                       MutHandle<VM::LinearString *> result)
{
    WH_ASSERT(!VM::IsInt32IdString(string));

    // Check if |string| is already a LinearString and marked as interned.
    if (string->isLinearString() && string->toLinearString()->isInterned()) {
        result = string->toLinearString();
        return true;
    }

    // Check for existing interned string in table.
    uint32_t slot = lookupSlot(StringOrQuery(string), &result.get());
    if (result)
        return true;

    // Allocate tenured LinearString copy (marked interned).
    uint32_t size = string->length() * 2;
    result = cx_->inTenured().createSized<VM::LinearString>(
                            size, string, /*interned=*/true);
    if (!result)
        return false;

    return insertString(result, slot);
}

bool
StringTable::addString(Handle<Value> strval,
                       MutHandle<VM::LinearString *> result)
{
    WH_ASSERT(strval->isString());
    WH_ASSERT(!VM::IsInt32IdString(strval));

    if (strval->isImmString()) {
        uint16_t buf[Value::ImmStringMaxLength];
        uint32_t len = strval->readImmString(buf);
        return addString(buf, len, result);
    }

    WH_ASSERT(strval->isHeapString());
    Root<VM::HeapString *> heapStr(cx_, strval->heapStringPtr());
    return addString(heapStr, result);
}


uint32_t
StringTable::lookupSlot(const StringOrQuery &str, VM::LinearString **result)
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
            VM::HeapString *heapStr = slotVal->heapStringPtr();
            WH_ASSERT(heapStr->isLinearString());
            VM::LinearString *linearStr = heapStr->toLinearString();

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
    uint32_t spoiler = cx_->spoiler();

    if (str.isQuery()) {
        const Query *query = str.toQuery();
        if (query->isEightBit) {
            return VM::FNVHashString(spoiler, query->eightBitData(),
                                     query->length);
        }

        return VM::FNVHashString(spoiler, query->sixteenBitData(),
                                 query->length);
    }

    WH_ASSERT(str.isHeapString());
    const VM::HeapString *heapStr = str.toHeapString();

    if (heapStr->isLinearString()) {
        const VM::LinearString *linStr = heapStr->toLinearString();
        return VM::FNVHashString(spoiler, linStr->data(), linStr->length());
    }

    return VM::FNVHashString(spoiler, heapStr);
}

int
StringTable::compareStrings(VM::LinearString *a,
                            const StringOrQuery &b)
{
    if (b.isQuery()) {
        const Query *query = b.toQuery();
        if (query->isEightBit) {
            return VM::CompareStrings(a->data(), a->length(),
                                      query->eightBitData(), query->length);
        }
        return VM::CompareStrings(a->data(), a->length(),
                                  query->sixteenBitData(), query->length);
    }

    WH_ASSERT(b.isHeapString());
    const VM::HeapString *heapStr = b.toHeapString();

    if (heapStr->isLinearString()) {
        const VM::LinearString *linStr = heapStr->toLinearString();
        return VM::CompareStrings(a->data(), a->length(),
                                  linStr->data(), linStr->length());
    }
    return VM::CompareStrings(a->data(), a->length(), heapStr);
}

bool
StringTable::insertString(Handle<VM::LinearString *> str, uint32_t slot)
{
    WH_ASSERT(tuple_->get(slot)->isUndefined());
    WH_ASSERT(str->isInterned());

    // Resize table if necessary.
    if (entries_ >= tuple_->size() * MAX_FILL_RATIO) {
        if (!enlarge())
            return false;

        VM::LinearString *exist;
        slot = lookupSlot(StringOrQuery(str), &exist);
        WH_ASSERT(!exist);
    }

    // Store interned string.
    tuple_->set(slot, Value::HeapString(str));
    entries_++;
    return true;
}

bool
StringTable::enlarge()
{
    Root<VM::Tuple *> oldTuple(cx_, tuple_);
    uint32_t curSize = tuple_->size();
    
    // Allocate a new tuple with double capacity.
    if (!cx_->inTenured().createTuple(curSize * 2, tuple_))
        return false;

    // Add old strings to table.
    for (uint32_t i = 0; i < curSize; i++) {
        Handle<Value> oldVal = tuple_->get(i);
        WH_ASSERT(oldVal->isUndefined() || oldVal->isFalse() ||
                  oldVal->isHeapString());
        if (!oldVal->isHeapString())
            continue;

        WH_ASSERT(oldVal->heapStringPtr()->isLinearString());
        VM::LinearString *oldStr = oldVal->heapStringPtr()->toLinearString();
        

        // Check for existing interned string in table.
        VM::LinearString *dummy;
        uint32_t slot = lookupSlot(StringOrQuery(oldStr), &dummy);
        WH_ASSERT(dummy == nullptr);

        tuple_->set(slot, Value::HeapString(oldStr));
    }

    return true;
}


} // namespace Whisper

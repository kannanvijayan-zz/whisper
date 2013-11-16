#ifndef WHISPER__STRING_TABLE_HPP
#define WHISPER__STRING_TABLE_HPP

#include <vector>

#include "common.hpp"
#include "debug.hpp"
#include "rooting.hpp"

namespace Whisper {

class RunContext;
class ThreadContext;

namespace VM
{
    class HeapString;
    class LinearString;
    class Tuple;
}

//
// StringTable keeps a table of interned strings.
//
// All interned strings are LinearStrings.  Any non-interned
// LinearString added to the table (except for strings which are
// representable as ImmStrings), are copied into new LinearStrings
// and returned.
//
// When a string is interned, a new LinearString is created with the
// contents and added to the table, even if the incoming
// string is a LinearString.  This is because the interned string
// will be created in the tenured generation, so it contributes less
// to GC pressure, and the query string can be garbage collected
// earlier (e.g. from the nursery).
//

class StringTable
{
  private:
    // Query is a stack-allocated structure used represent
    // a length and a string pointer.
    struct alignas(2) Query {
        const void *data;
        uint32_t length;
        bool isEightBit;

        Query(const uint8_t *data, uint32_t length);
        Query(const uint16_t *data, uint32_t length);

        const uint8_t *eightBitData() const;
        const uint16_t *sixteenBitData() const;
    };

    // StringOrQuery can either be a pointer to a HeapString, or
    // a pointer to a stack-allocated StringTable::Query.
    // They are differentiated by the low bit of the pointer.
    struct StringOrQuery {
        uintptr_t ptr;

        StringOrQuery(const VM::HeapString *str);
        StringOrQuery(const Query *str);

        bool isHeapString() const;
        bool isQuery() const;

        const VM::HeapString *toHeapString() const;
        const Query *toQuery() const;
    };

    static constexpr uint32_t INITIAL_TUPLE_SIZE = 512;
    static constexpr float MAX_FILL_RATIO = 0.75;

    ThreadContext *cx_;
    uint32_t entries_;
    VM::Tuple *tuple_;

  public:
    StringTable();

    bool initialize(ThreadContext *cx);

    VM::LinearString *lookupString(const Value &val);
    VM::LinearString *lookupString(VM::HeapString *str);

    VM::LinearString *lookupString(const uint8_t *str, uint32_t length);
    VM::LinearString *lookupString(const uint16_t *str, uint32_t length);

    bool addString(const uint8_t *str, uint32_t length,
                   MutHandle<VM::LinearString *> result);
    bool addString(const uint16_t *str, uint32_t length,
                   MutHandle<VM::LinearString *> result);
    bool addString(Handle<VM::HeapString *> string,
                   MutHandle<VM::LinearString *> result);
    bool addString(Handle<Value> strval,
                   MutHandle<VM::LinearString *> result);

  private:
    uint32_t lookupSlot(const StringOrQuery &str, VM::LinearString **result);

    uint32_t hashString(const StringOrQuery &str);
    int compareStrings(VM::LinearString *a, const StringOrQuery &b);

    bool insertString(Handle<VM::LinearString *> str, uint32_t slot);
    bool enlarge();
};


} // namespace Whisper

#endif // WHISPER__STRING_TABLE_HPP

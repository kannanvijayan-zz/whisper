#ifndef WHISPER__VM__STRING_TABLE_HPP
#define WHISPER__VM__STRING_TABLE_HPP

#include <vector>

#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"
#include "vm/string.hpp"
#include "vm/tuple.hpp"

namespace Whisper {
namespace VM {


//
// StringTable keeps a table of interned strings.  All strings which
// are bound as property names are interned.
// Any two interned strings can be equality compared by comparing their
// pointer values.
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

        Query(uint32_t length, const uint8_t *data);
        Query(uint32_t length, const uint16_t *data);
    };

    // StringOrQuery can either be a pointer to a HeapString, or
    // a pointer to a stack-allocated StringTable::Query.
    // They are differentiated by the low bit of the pointer.
    struct StringOrQuery {
        uintptr_t ptr;

        StringOrQuery(const HeapString *str);
        StringOrQuery(const Query *str);

        bool isHeapString() const;
        bool isQuery() const;

        const HeapString *toHeapString() const;
        const Query *toQuery() const;
    };

    // String hasher.
    struct Hash {
        StringTable *table;

        Hash(StringTable *table);
        size_t operator ()(const StringOrQuery &str);
    };

    uint32_t spoiler_;
    uint32_t entries_;
    Tuple *tuple_;

  public:
    StringTable(uint32_t spoiler);

    bool initialize(RunContext *cx);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_TABLE_HPP

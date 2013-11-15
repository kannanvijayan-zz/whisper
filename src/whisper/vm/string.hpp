#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP

#include "common.hpp"
#include "debug.hpp"
#include "rooting.hpp"
#include "vm/heap_type_defn.hpp"
#include "vm/heap_thing.hpp"

namespace Whisper {
namespace VM {

//
// Strings have a number of different representations, modeled by
// a number of different concrete string classes.
//
// The abstract String class is used as a type that all of them
// can be implicitly cast to, to provide a general interface to
// all of them.
//
struct HeapString : public HeapThing
{
  protected:
    HeapString();
    HeapString(const HeapString &name) = delete;

    const HeapThing *toHeapThing() const;
    HeapThing *toHeapThing();

  public:
#if defined(ENABLE_DEBUG)
    bool isValidString() const;
#endif

    bool isLinearString() const;
    const LinearString *toLinearString() const;
    LinearString *toLinearString();

    uint32_t length() const;
    uint16_t getChar(uint32_t idx) const;
    uint32_t extract(uint32_t buflen, uint16_t *buf);

    bool fitsImmediate() const;
};


//
// LinearString is a string representation which embeds all the 16-bit
// characters within the object.
//
//      +-----------------------+
//      | Header                |
//      +-----------------------+
//      | String Data           |
//      | ...                   |
//      | ...                   |
//      +-----------------------+
//
//  Flags
//      Interned - indicates if string is interned in the string table.
//
class LinearString : public HeapString,
                     public TypedHeapThing<HeapType::LinearString>
{
  friend class HeapString;
  public:
    static constexpr uint32_t InternedFlagMask = 0x1;

  private:
    void initializeFlags(bool interned);
    uint16_t *writableData();
    
  public:
    LinearString(const HeapString *str, bool interned = false);
    LinearString(const uint8_t *data, bool interned = false);
    LinearString(const uint16_t *data, bool interned = false);

    const uint16_t *data() const;

    bool isInterned() const;

    uint32_t length() const;
    uint16_t getChar(uint32_t idx) const;
    uint32_t extract(uint32_t buflen, uint16_t *buf);
};


//
// Unpacking helper class for strings.
//

class StringUnpack
{
  private:
    union {
        struct {
            uint8_t data[Value::ImmString8MaxLength];
        } str8;
        struct {
            uint16_t data[Value::ImmString16MaxLength];
        } str16;
        struct {
            uint8_t data[Value::ImmIndexStringMaxLength];
        } idxStr;
    } immData_;

    static constexpr uint8_t IS_LINEAR = 0x01;
    static constexpr uint8_t IS_EIGHT_BIT = 0x02;

    uint8_t flags_;

    union {
        const void *charData_;
        HeapString *heapStr_;
    };
    uint32_t length_;

  private:
    void init(HeapString *heapStr);

  public:
    StringUnpack(const Value &val);
    StringUnpack(HeapString *heapStr);

    uint32_t length() const;

    bool hasEightBit() const;
    bool hasSixteenBit() const;
    bool isNonLinear() const;

    const uint8_t *eightBitData() const;
    const uint16_t *sixteenBitData() const;
    HeapString *heapString() const;
};


//
// String hashing.
//

uint32_t FNVHashString(uint32_t spoiler, const Value &strVal);
uint32_t FNVHashString(uint32_t spoiler, const HeapString *heapStr);
uint32_t FNVHashString(uint32_t spoiler, const uint8_t *str, uint32_t length);
uint32_t FNVHashString(uint32_t spoiler, const uint16_t *str, uint32_t length);

//
// String comparison.
//

int CompareStrings(const Value &strA, const uint8_t *strB, uint32_t lengthB);
int CompareStrings(const uint8_t *strA, uint32_t lengthA, const Value &strB);

int CompareStrings(const Value &strA, const uint16_t *strB, uint32_t lengthB);
int CompareStrings(const uint16_t *strA, uint32_t lengthA, const Value &strB);

int CompareStrings(const HeapString *strA,
                   const uint8_t *strB, uint32_t lengthB);

int CompareStrings(const uint8_t *strA, uint32_t lengthA,
                   const HeapString *strB);

int CompareStrings(const HeapString *strA,
                   const uint16_t *strB, uint32_t lengthB);

int CompareStrings(const uint16_t *strA, uint32_t lengthA,
                   const HeapString *strB);

int CompareStrings(const Value &strA, const HeapString *strB);
int CompareStrings(const HeapString *strA, const Value &strB);

int CompareStrings(const Value &strA, const Value &strB);
int CompareStrings(const HeapString *strA, const HeapString *strB);

int CompareStrings(const uint8_t *strA, uint32_t lengthA,
                   const uint8_t *strB, uint32_t lengthB);

int CompareStrings(const uint16_t *strA, uint32_t lengthA,
                   const uint16_t *strB, uint32_t lengthB);

int CompareStrings(const uint8_t *strA, uint32_t lengthA,
                   const uint16_t *strB, uint32_t lengthB);

int CompareStrings(const uint16_t *strA, uint32_t lengthA,
                   const uint8_t *strB, uint32_t lengthB);

//
// Check string for id.
//
bool IsInt32IdString(const uint8_t *str, uint32_t length,
                     int32_t *val=nullptr);
bool IsInt32IdString(const uint16_t *str, uint32_t length,
                     int32_t *val=nullptr);
bool IsInt32IdString(HeapString *str, int32_t *val=nullptr);
bool IsInt32IdString(const Value &strval, int32_t *val=nullptr);


//
// Normalize a string.  Return either an immediate index string, or
// an interned linear property name string.
//
bool NormalizeString(RunContext *cx, const uint8_t *str, uint32_t length,
                     MutHandle<Value> result);
bool NormalizeString(RunContext *cx, const uint16_t *str, uint32_t length,
                     MutHandle<Value> result);
bool NormalizeString(RunContext *cx, Handle<HeapString *> str,
                     MutHandle<Value> result);
bool NormalizeString(RunContext *cx, Handle<Value> strval,
                     MutHandle<Value> result);


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_HPP

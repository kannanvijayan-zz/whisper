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
struct HeapString
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

    bool linearize(RunContext *cx, MutHandle<LinearString *> out);

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
class LinearString : public HeapThing,
                     public HeapString,
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
bool IsInt32IdString(const uint8_t *str, uint32_t length, int32_t *val);
bool IsInt32IdString(const uint16_t *str, uint32_t length, int32_t *val);
bool IsInt32IdString(HeapString *str, int32_t *val);

} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_HPP

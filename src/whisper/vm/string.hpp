#ifndef WHISPER__VM__STRING_HPP
#define WHISPER__VM__STRING_HPP

#include "common.hpp"
#include "debug.hpp"
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

    bool fitsImmediate() const;
};


//
// LinearString objects can be composed of 8 or 16 bit characters.
// The character class is identified by the low-bit flag on
// the string.
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
//      EightBit - indicates if string contains 8 or 16 bit chars.
//      Interned - indicates if string is interned in the string table.
//      PropertyName - indicates if the string is a property name.
//
struct LinearString : public HeapThing,
                      public HeapString,
                      public TypedHeapThing<HeapType::LinearString>
{
  public:
    static constexpr uint32_t EightBitFlagMask = 0x1;
    static constexpr uint32_t InternedFlagMask = 0x2;
    static constexpr uint32_t PropertyNameFlagMask = 0x4;

  protected:
    uint8_t *writableEightBitData();
    uint16_t *writableSixteenBitData();
    
  public:
    LinearString(const uint8_t *data);
    LinearString(const uint16_t *data);
    LinearString(const uint8_t *data, bool interned, bool propName);
    LinearString(const uint16_t *data, bool interned, bool propName);

    bool isEightBit() const;
    const uint8_t *eightBitData() const;

    bool isSixteenBit() const;
    const uint16_t *sixteenBitData() const;

    bool isInterned() const;
    bool isPropertyName() const;

    uint32_t length() const;
    uint16_t getChar(uint32_t idx) const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_HPP

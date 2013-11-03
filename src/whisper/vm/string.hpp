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
//      Group - one of Unknown, Index, or Name.  Identifies whether the
//          string is a known index, known non-index (name), or not yet
//          known.
//
class LinearString : public HeapThing,
                     public HeapString,
                     public TypedHeapThing<HeapType::LinearString>
{
  friend class HeapString;
  public:
    static constexpr uint32_t InternedFlagMask = 0x1;
    static constexpr unsigned GroupShift = 1;
    static constexpr unsigned GroupMask = 0x3;

    enum class Group : uint8_t { Unknown = 0, Index = 1, Name = 2 };

  private:
    void initializeFlags(bool interned, Group group);
    uint16_t *writableData();
    
  public:
    LinearString(const HeapString *str, bool interned = false,
                 Group group = Group::Unknown);
    LinearString(const uint8_t *data, bool interned = false,
                 Group group = Group::Unknown);
    LinearString(const uint16_t *data, bool interned = false,
                 Group group = Group::Unknown);

    const uint16_t *data() const;

    bool isInterned() const;

    Group group() const;
    bool inUnknownGroup() const;
    bool inIndexGroup() const;
    bool inNameGroup() const;

    uint32_t length() const;
    uint16_t getChar(uint32_t idx) const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__STRING_HPP

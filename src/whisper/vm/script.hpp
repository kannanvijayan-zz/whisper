#ifndef WHISPER__VM__SCRIPT_HPP
#define WHISPER__VM__SCRIPT_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "vm/heap_thing.hpp"
#include "vm/bytecode.hpp"

#include <limits>
#include <algorithm>

namespace Whisper {
namespace VM {


//
// Scripts represent the executable code for a function or top-level
// script.
//
//      +-----------------------+
//      | Header                |
//      +-----------------------+
//      | Bytecode              |
//      +-----------------------+
//
// The header flags for this object are used to store the following
// information:
//  strict - whether the script executes in strict mode.
//  type - one of {Global, Function, Eval}
//
//
struct Script : public HeapThing, public TypedHeapThing<HeapType::Script>
{
  public:
    enum Flags : uint32_t
    {
        IsStrict    = 0x01
    };

    enum Mode : uint32_t
    {
        Global,
        Function,
        Eval
    };

    static constexpr unsigned ModeBits = 2;
    static constexpr uint32_t ModeMask = 0x3;
    static constexpr unsigned ModeShift = 1;

    struct Config
    {
        bool isStrict;
        Mode mode;

        Config(bool isStrict, Mode mode) : isStrict(isStrict), mode(mode) {}
    };

  private:
    // Pointer to bytecode for the script.
    HeapThingValue<Bytecode> bytecode_;

    void initialize(const Config &config);

  public:
    Script(Bytecode *bytecode, const Config &config);

    bool isStrict() const;

    Mode mode() const;

    Bytecode *bytecode() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SCRIPT_HPP

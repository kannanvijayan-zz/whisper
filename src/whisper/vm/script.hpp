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
//      | Info                  |
//      +-----------------------+
//      | Bytecode              |
//      +-----------------------+
//
// The first word is a MagicValue holding:
//  MaxStackDepth - the maximum value stack depth required by the bytecode.
//
// The header flags for this object are used to store the following
// information:
//  strict - whether the script executes in strict mode.
//  mode - one of {TopLevel, Function, Eval}
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
        TopLevel,
        Function,
        Eval
    };

    static constexpr unsigned ModeBits = 2;
    static constexpr uint32_t ModeMask = 0x3;
    static constexpr unsigned ModeShift = 1;

    static constexpr unsigned MaxStackDepthBits = 20;
    static constexpr unsigned MaxStackDepthShift = 20;
    static constexpr uint64_t MaxStackDepthMaskLow =
        (UInt64(1) << MaxStackDepthBits) - 1;

    struct Config
    {
        bool isStrict;
        Mode mode;
        uint32_t maxStackDepth;

        Config(bool isStrict, Mode mode, uint32_t maxStackDepth)
          : isStrict(isStrict), mode(mode), maxStackDepth(maxStackDepth)
        {}
    };

  private:
    Value info_;

    // Pointer to bytecode for the script.
    HeapThingValue<Bytecode> bytecode_;

    void initialize(const Config &config);

  public:
    Script(Bytecode *bytecode, const Config &config);

    bool isStrict() const;

    Mode mode() const;

    bool isTopLevel() const;
    bool isFunction() const;
    bool isEval() const;

    uint32_t maxStackDepth() const;

    Bytecode *bytecode() const;
};

typedef HeapThingWrapper<Script> WrappedScript;
typedef Root<Script *> RootedScript;
typedef Handle<Script *> HandleScript;
typedef MutableHandle<Script *> MutHandleScript;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SCRIPT_HPP

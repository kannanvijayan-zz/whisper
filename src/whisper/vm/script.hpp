#ifndef WHISPER__VM__SCRIPT_HPP
#define WHISPER__VM__SCRIPT_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "vm/heap_thing.hpp"
#include "vm/bytecode.hpp"
#include "vm/tuple.hpp"

#include <limits>
#include <algorithm>

namespace Whisper {
namespace VM {


//
// Scripts represent the executable code for a function or top-level
// script.
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
    Heap<Bytecode *> bytecode_;
    Heap<Tuple *> constants_;
    uint32_t maxStackDepth_;

    void initialize(const Config &config);

  public:
    Script(Bytecode *bytecode, Tuple *constants, const Config &config);

    bool isStrict() const;

    Mode mode() const;

    bool isTopLevel() const;
    bool isFunction() const;
    bool isEval() const;

    Handle<Bytecode *> bytecode() const;
    Handle<Tuple *> constants() const;

    uint32_t maxStackDepth() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__SCRIPT_HPP

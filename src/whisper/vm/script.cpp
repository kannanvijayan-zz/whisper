#include "value_inlines.hpp"
#include "vm/script.hpp"
#include "vm/heap_thing_inlines.hpp"

namespace Whisper {
namespace VM {

//
// Script
//

void
Script::initialize(const Config &config)
{
    uint32_t flags = 0;
    if (config.isStrict)
        flags |= IsStrict;
    flags |= static_cast<uint32_t>(config.mode) << ModeShift;
    initFlags(flags);
}

Script::Script(Bytecode *bytecode, const Config &config)
  : bytecode_(bytecode)
{
    initialize(config);
}

bool
Script::isStrict() const
{
    return flags() & IsStrict;
}

Script::Mode
Script::mode() const
{
    return static_cast<Mode>((flags() >> ModeShift) & ModeMask);
}

bool
Script::isTopLevel() const
{
    return mode() == TopLevel;
}

bool
Script::isFunction() const
{
    return mode() == Function;
}

bool
Script::isEval() const
{
    return mode() == Eval;
}

Bytecode *
Script::bytecode() const
{
    return bytecode_;
}


} // namespace VM
} // namespace Whisper

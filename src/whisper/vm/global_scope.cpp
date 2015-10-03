
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "interp/interpreter.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<GlobalScope*>
GlobalScope::Create(AllocationContext acx)
{
    // Allocate empty array of delegates.
    Local<Array<Wobject*>*> delegates(acx);
    if (!delegates.setResult(Array<Wobject*>::CreateEmpty(acx)))
        return ErrorVal();

    // Allocate a dictionary.
    Local<PropertyDict*> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return ErrorVal();

    // Allocate global.
    Local<GlobalScope*> global(acx);
    if (!global.setResult(acx.create<GlobalScope>(delegates.handle(),
                                                  props.handle())))
    {
        return ErrorVal();
    }

    return OkVal(global.get());
}

WobjectHooks const*
GlobalScope::getGlobalScopeHooks() const
{
    return hashObjectHooks();
}


} // namespace VM
} // namespace Whisper

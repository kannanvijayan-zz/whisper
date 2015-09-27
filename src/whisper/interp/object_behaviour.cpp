
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "vm/plain_object.hpp"
#include "interp/interpreter.hpp"
#include "interp/object_behaviour.hpp"

namespace Whisper {
namespace Interp {


Result<VM::Wobject*>
CreateRootDelegate(AllocationContext acx)
{
    // Create an empty array of delegates.
    Local<VM::Array<VM::Wobject*>*> delegates(acx);
    if (!delegates.setResult(VM::Array<VM::Wobject*>::CreateEmpty(acx)))
        return ErrorVal();

    // Create a plain object.
    Local<VM::PlainObject*> plainObj(acx);
    if (!plainObj.setResult(VM::PlainObject::Create(acx, delegates)))
        return ErrorVal();

    return OkVal(plainObj.get());
}


} // namespace Interp
} // namespace Whisper

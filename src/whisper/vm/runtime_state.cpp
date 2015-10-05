
#include "runtime_inlines.hpp"
#include "vm/runtime_state.hpp"
#include "vm/global_scope.hpp"
#include "vm/wobject.hpp"
#include "interp/syntax_behaviour.hpp"
#include "interp/object_behaviour.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<RuntimeState*>
RuntimeState::Create(AllocationContext acx)
{
    // Allocate the array.
    Local<String*> vmstr(acx, nullptr);
    Local<Array<String*>*> namePool(acx);
    if (!namePool.setResult(
            Array<String*>::CreateFill(acx, NamePool::Size(), vmstr.handle())))
    {
        return ErrorVal();
    }

#define ALLOC_STRING_(name, str) \
    if (!vmstr.setResult(VM::String::Create(acx, str))) \
        return ErrorVal(); \
    namePool->set(NamePool::IndexOfId(NamePool::Id::name), vmstr.get());
        WHISPER_DEFN_NAME_POOL(ALLOC_STRING_)
#undef ALLOC_STRING_

    return acx.create<RuntimeState>(namePool.get());
}

/* static */ Result<ThreadState*>
ThreadState::Create(AllocationContext acx)
{
    // Initialize the global.
    Local<VM::GlobalScope*> glob(acx);
    if (!glob.setResult(VM::GlobalScope::Create(acx)))
        return ErrorVal();

    if (!Interp::BindSyntaxHandlers(acx, glob))
        return ErrorVal();

    // Initialize the root delegate.
    Local<VM::Wobject*> rootDelegate(acx);
    if (!rootDelegate.setResult(Interp::CreateRootDelegate(acx)))
        return ErrorVal();

    // Initialize the immediate integer delegate.
    Local<VM::Wobject*> immIntDelegate(acx);
    if (!immIntDelegate.setResult(
            Interp::CreateImmIntDelegate(acx, rootDelegate)))
    {
        return ErrorVal();
    }

    // Initialize the immediate boolean delegate.
    Local<VM::Wobject*> immBoolDelegate(acx);
    if (!immBoolDelegate.setResult(
            Interp::CreateImmBoolDelegate(acx, rootDelegate)))
    {
        return ErrorVal();
    }

    return acx.create<ThreadState>(glob.handle(), rootDelegate.handle(),
                                   immIntDelegate.handle(),
                                   immBoolDelegate.handle());
}


} // namespace VM
} // namespace Whisper

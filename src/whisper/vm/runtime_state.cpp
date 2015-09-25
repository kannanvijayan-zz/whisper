
#include "runtime_inlines.hpp"
#include "vm/runtime_state.hpp"

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


} // namespace VM
} // namespace Whisper

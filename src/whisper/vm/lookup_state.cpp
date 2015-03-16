
#include "runtime_inlines.hpp"
#include "vm/lookup_state.hpp"

namespace Whisper {
namespace VM {


LookupNode *
LookupNode::Create(AllocationContext acx,
                   Handle<LookupNode *> parent,
                   Handle<Wobject *> object,
                   Handle<Array<Wobject *> *> delegates,
                   uint32_t index)
{
    return acx.create<LookupNode>(parent.get(), object.get(),
                                  delegates.get(), 0);
}

LookupState *
LookupState::Create(AllocationContext acx,
                    Handle<Wobject *> receiver,
                    Handle<String *> name,
                    Handle<LookupNode *> node)
{
    return acx.create<LookupState>(receiver.get(), name.get(), node.get());
}


} // namespace VM
} // namespace Whisper

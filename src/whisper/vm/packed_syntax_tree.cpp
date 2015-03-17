
#include "runtime_inlines.hpp"
#include "vm/array.hpp"
#include "vm/packed_syntax_tree.hpp"

namespace Whisper {
namespace VM {


/* static */ PackedSyntaxTree *
PackedSyntaxTree::Create(AllocationContext acx,
                         ArrayHandle<uint32_t> data,
                         ArrayHandle<Box> constPool)
{
    // Allocate data array.
    Local<Array<uint32_t> *> dataArray(acx,
        Array<uint32_t>::Create(acx, data));
    if (!dataArray.get())
        return nullptr;

    // Allocate data array.
    Local<Array<Box> *> constPoolArray(acx,
        Array<Box>::Create(acx, constPool));
    if (!constPoolArray.get())
        return nullptr;

    return acx.create<PackedSyntaxTree>(dataArray.handle(),
                                        constPoolArray.handle());
}


} // namespace VM
} // namespace Whisper

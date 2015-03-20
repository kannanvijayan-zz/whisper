
#include "runtime_inlines.hpp"
#include "vm/array.hpp"
#include "vm/packed_syntax_tree.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PackedSyntaxTree *>
PackedSyntaxTree::Create(AllocationContext acx,
                         ArrayHandle<uint32_t> data,
                         ArrayHandle<Box> constPool)
{
    // Allocate data array.
    Local<Array<uint32_t> *> dataArray(acx);
    if (!dataArray.setResult(Array<uint32_t>::Create(acx, data)))
        return Result<PackedSyntaxTree *>::Error();

    // Allocate constPool array.
    Local<Array<Box> *> constPoolArray(acx);
    if (!constPoolArray.setResult(Array<Box>::Create(acx, constPool)))
        return Result<PackedSyntaxTree *>::Error();

    return acx.create<PackedSyntaxTree>(dataArray.handle(),
                                        constPoolArray.handle());
}


} // namespace VM
} // namespace Whisper

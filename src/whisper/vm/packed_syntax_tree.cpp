
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
    if (!dataArray.setResult(Array<uint32_t>::CreateCopy(acx, data)))
        return ErrorVal();

    // Allocate constPool array.
    Local<Array<Box> *> constPoolArray(acx);
    if (!constPoolArray.setResult(Array<Box>::CreateCopy(acx, constPool)))
        return ErrorVal();

    return acx.create<PackedSyntaxTree>(dataArray.handle(),
                                        constPoolArray.handle());
}

/* static */ Result<SyntaxTreeFragment *>
SyntaxTreeFragment::Create(AllocationContext acx,
                           Handle<PackedSyntaxTree *> pst,
                           uint32_t offset)
{
    return acx.create<SyntaxTreeFragment>(pst, offset);
}


} // namespace VM
} // namespace Whisper

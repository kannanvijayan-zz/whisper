
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
    Result<Array<uint32_t> *> maybeDataArray =
        Array<uint32_t>::Create(acx, data);
    if (!maybeDataArray)
        return Result<PackedSyntaxTree *>::Error();

    Local<Array<uint32_t> *> dataArray(acx, maybeDataArray.value());

    // Allocate data array.
    Result<Array<Box> *> maybeConstPoolArray =
        Array<Box>::Create(acx, constPool);
    if (!maybeConstPoolArray)
        return Result<PackedSyntaxTree *>::Error();
        
    Local<Array<Box> *> constPoolArray(acx, maybeConstPoolArray.value());

    return acx.create<PackedSyntaxTree>(dataArray.handle(),
                                        constPoolArray.handle());
}


} // namespace VM
} // namespace Whisper

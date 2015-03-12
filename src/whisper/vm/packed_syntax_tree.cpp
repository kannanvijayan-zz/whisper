
#include "runtime_inlines.hpp"
#include "vm/array.hpp"
#include "vm/packed_syntax_tree.hpp"

namespace Whisper {
namespace VM {


/* static */ PackedSyntaxTree *
PackedSyntaxTree::Create(AllocationContext acx,
                         uint32_t dataSize,
                         const uint32_t *data,
                         uint32_t constPoolSize,
                         const Box *constPool)
{
    ThreadContext *cx = acx.threadContext();

    // Allocate data array.
    Local<Array<uint32_t> *> dataArray(cx,
        Array<uint32_t>::Create(acx, dataSize, data));
    if (!dataArray.get())
        return nullptr;

    // Allocate data array.
    Local<Array<Box> *> constPoolArray(cx,
        Array<Box>::Create(acx, constPoolSize, constPool));
    if (!constPoolArray.get())
        return nullptr;

    return acx.create<PackedSyntaxTree>(dataArray.get(),
                                        constPoolArray.get());
}


} // namespace VM
} // namespace Whisper

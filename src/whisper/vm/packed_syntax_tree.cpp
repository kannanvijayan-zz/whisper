
#include "runtime_inlines.hpp"
#include "vm/array.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "parser/syntax_tree.hpp"
#include "parser/packed_syntax.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<PackedSyntaxTree*>
PackedSyntaxTree::Create(AllocationContext acx,
                         ArrayHandle<uint32_t> data,
                         ArrayHandle<Box> constPool)
{
    // Allocate data array.
    Local<Array<uint32_t>*> dataArray(acx);
    if (!dataArray.setResult(Array<uint32_t>::CreateCopy(acx, data)))
        return ErrorVal();

    // Allocate constPool array.
    Local<Array<Box>*> constPoolArray(acx);
    if (!constPoolArray.setResult(Array<Box>::CreateCopy(acx, constPool)))
        return ErrorVal();

    return acx.create<PackedSyntaxTree>(dataArray.handle(),
                                        constPoolArray.handle());
}

/* static */ Result<SyntaxTreeFragment*>
SyntaxTreeFragment::Create(AllocationContext acx,
                           Handle<PackedSyntaxTree*> pst,
                           uint32_t offset)
{
    return acx.create<SyntaxTreeFragment>(pst, offset);
}

AST::NodeType
SyntaxTreeFragment::nodeType() const
{
    // Create a packed syntax node.
    AST::PackedBaseNode node(pst_->data(), offset_);
    return node.type();
}

char const*
SyntaxTreeFragment::nodeTypeCString() const
{
    return AST::NodeTypeString(nodeType());
}

AST::NodeType
SyntaxTreeRef::nodeType() const
{
    return AST::PackedBaseNode(pst_->data(), offset_).type();
}

char const*
SyntaxTreeRef::nodeTypeCString() const
{
    return AST::NodeTypeString(nodeType());
}


} // namespace VM
} // namespace Whisper

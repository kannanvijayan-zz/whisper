
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

AST::NodeType
SyntaxNodeRef::nodeType() const
{
    WH_ASSERT(isValid());
    return AST::PackedBaseNode(pst_->data(), offset_).type();
}

char const*
SyntaxNodeRef::nodeTypeCString() const
{
    WH_ASSERT(isValid());
    return AST::NodeTypeString(nodeType());
}

/* static */ Result<SyntaxNode*>
SyntaxNode::Create(AllocationContext acx,
                   Handle<PackedSyntaxTree*> pst,
                   uint32_t offset)
{
    return acx.create<SyntaxNode>(pst, offset);
}

AST::NodeType
SyntaxNode::nodeType() const
{
    // Create a packed syntax node.
    return AST::PackedBaseNode(pst_->data(), offset_).type();
}

char const*
SyntaxNode::nodeTypeCString() const
{
    return AST::NodeTypeString(nodeType());
}


/* static */ Result<SyntaxBlock*>
SyntaxBlock::Create(AllocationContext acx,
                   Handle<PackedSyntaxTree*> pst,
                   uint32_t offset,
                   uint32_t numStatements)
{
    return acx.create<SyntaxBlock>(pst, offset, numStatements);
}


} // namespace VM
} // namespace Whisper

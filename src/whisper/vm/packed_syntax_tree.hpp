#ifndef WHISPER__VM__PACKED_SYNTAX_TREE_HPP
#define WHISPER__VM__PACKED_SYNTAX_TREE_HPP

#include "parser/syntax_tree.hpp"
#include "parser/packed_syntax.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace VM {


//
// A PackedSyntaxTree contains mappings from symbols to the location within the
// file that contains the symbol definition.
//
class PackedSyntaxTree
{
    friend struct TraceTraits<PackedSyntaxTree>;

  private:
    HeapField<Array<uint32_t>*> data_;
    HeapField<Array<Box>*> constants_;

  public:
    PackedSyntaxTree(Array<uint32_t>* data,
                     Array<Box>* constants)
      : data_(data),
        constants_(constants)
    {
        WH_ASSERT(data != nullptr);
        WH_ASSERT(constants != nullptr);
    }

    static Result<PackedSyntaxTree*> Create(AllocationContext acx,
                                            ArrayHandle<uint32_t> data,
                                            ArrayHandle<Box> constPool);

    Array<uint32_t>* data() const {
        return data_;
    }
    uint32_t dataSize() const {
        return data_->length();
    }

    uint32_t numConstants() const {
        return constants_->length();
    }
    Array<Box>* constants() const {
        return constants_;
    }

    uint32_t startOffset() const {
        return 0;
    }

    Box getConstant(uint32_t idx) const {
        WH_ASSERT(idx < numConstants());
        return constants_->get(idx);
    }
    VM::String* getConstantString(uint32_t idx) const {
        WH_ASSERT(idx < numConstants());
        Box box = getConstant(idx);
        WH_ASSERT(box.isPointer());
        WH_ASSERT(box.pointer<HeapThing>()->isString());
        return box.pointer<VM::String>();
    }

    AST::PackedBaseNode astBaseNodeAt(uint32_t offset) {
        WH_ASSERT(offset < dataSize());
        return AST::PackedBaseNode(data_, offset);
    }

#define VM_PACKEDST_GET_(ntype) \
    AST::Packed##ntype##Node ast##ntype##At(uint32_t offset) { \
        return astBaseNodeAt(offset).as##ntype(); \
    }
    WHISPER_DEFN_SYNTAX_NODES(VM_PACKEDST_GET_)
#undef VM_PACKEDST_GET_
};

//
// A SyntaxTreeRef is a on-stack reference to some element
// within a packed syntax tree.  It is subclassed by
// SyntaxNodeRef and SyntaxBlockRef.
//
class SyntaxTreeRef
{
    friend struct TraceTraits<SyntaxTreeRef>;

  protected:
    StackField<PackedSyntaxTree*> pst_;
    uint32_t offset_;
    uint32_t numStatements_;
    bool isBlock_;

    SyntaxTreeRef()
      : pst_(nullptr),
        offset_(0),
        numStatements_(0),
        isBlock_(false)
    {}

    SyntaxTreeRef(PackedSyntaxTree* pst, uint32_t offset)
      : pst_(pst),
        offset_(offset),
        numStatements_(0),
        isBlock_(false)
    {
        WH_ASSERT(pst_.get() != nullptr);
    }

    SyntaxTreeRef(PackedSyntaxTree* pst,
                  uint32_t offset,
                  uint32_t numStatements)
      : pst_(pst),
        offset_(offset),
        numStatements_(numStatements),
        isBlock_(true)
    {
        WH_ASSERT(pst_.get() != nullptr);
    }

    SyntaxTreeRef(PackedSyntaxTree* pst,
                  uint32_t offset,
                  uint32_t numStatements,
                  bool isBlock)
      : pst_(pst),
        offset_(offset),
        numStatements_(numStatements),
        isBlock_(isBlock)
    {
        WH_ASSERT_IF(!isBlock_, numStatements_ == 0);
    }

  public:
    SyntaxTreeRef(PackedSyntaxTree* pst, AST::PackedBaseNode const& node)
      : SyntaxTreeRef(pst, node.offset())
    {}

    SyntaxTreeRef(PackedSyntaxTree* pst, AST::PackedBlock const& packedBlock)
      : SyntaxTreeRef(pst, packedBlock.offset(), packedBlock.numStatements())
    {}

    SyntaxTreeRef(PackedSyntaxTree* pst,
                  AST::PackedSizedBlock const& packedSizedBlock)
      : SyntaxTreeRef(pst, packedSizedBlock.unsizedBlock())
    {}

    SyntaxTreeRef(SyntaxNode const* stNode);
    SyntaxTreeRef(SyntaxBlock const* stBlock);
    SyntaxTreeRef(SyntaxTreeFragment const* stFrag);

    bool isValid() const {
        return pst_.get() != nullptr;
    }
    bool isNode() const {
        WH_ASSERT(isValid());
        return !isBlock_;
    }
    bool isBlock() const {
        WH_ASSERT(isValid());
        return isBlock_;
    }

    Handle<PackedSyntaxTree*> pst() const {
        WH_ASSERT(isValid());
        return pst_;
    }
    uint32_t offset() const {
        WH_ASSERT(isValid());
        return offset_;
    }

    SyntaxNodeRef const* toNode() const {
        WH_ASSERT(isNode());
        return reinterpret_cast<SyntaxNodeRef const*>(this);
    }
    SyntaxNodeRef* toNode() {
        WH_ASSERT(isNode());
        return reinterpret_cast<SyntaxNodeRef*>(this);
    }

    SyntaxBlockRef const* toBlock() const {
        WH_ASSERT(isBlock());
        return reinterpret_cast<SyntaxBlockRef const*>(this);
    }
    SyntaxBlockRef* toBlock() {
        WH_ASSERT(isBlock());
        return reinterpret_cast<SyntaxBlockRef*>(this);
    }
};

//
// A SyntaxNodeRef is a on-stack reference to a node in
// a packed syntax tree.
//
class SyntaxNodeRef : public SyntaxTreeRef
{
    friend struct TraceTraits<SyntaxNodeRef>;

  public:
    SyntaxNodeRef() : SyntaxTreeRef() {}

    SyntaxNodeRef(PackedSyntaxTree* pst,
                  uint32_t offset)
      : SyntaxTreeRef(pst, offset)
    {}

    SyntaxNodeRef(PackedSyntaxTree* pst, AST::PackedBaseNode const& node)
      : SyntaxTreeRef(pst, node)
    {}

    SyntaxNodeRef(SyntaxNode const* stNode)
      : SyntaxTreeRef(stNode)
    {}

    AST::NodeType nodeType() const;
    char const* nodeTypeCString() const;

    AST::PackedBaseNode astBaseNode() {
        WH_ASSERT(isValid());
        return pst_->astBaseNodeAt(offset());
    }

#define VM_STREF_GET_(ntype) \
    AST::Packed##ntype##Node ast##ntype() { \
        return astBaseNode().as##ntype(); \
    }
    WHISPER_DEFN_SYNTAX_NODES(VM_STREF_GET_)
#undef VM_STREF_GET_

    // Helper to create a new SyntaxNode for this one.
    Result<SyntaxNode*> createSyntaxNode(AllocationContext acx);
};

//
// A SyntaxBlockRef is a on-stack reference to a block or
// sized block in a packed syntax tree.
//
class SyntaxBlockRef : public SyntaxTreeRef
{
    friend struct TraceTraits<SyntaxBlockRef>;
  public:
    SyntaxBlockRef() : SyntaxTreeRef() {}

    SyntaxBlockRef(PackedSyntaxTree* pst,
                   uint32_t offset,
                   uint32_t numStatements)
      : SyntaxTreeRef(pst, offset, numStatements)
    {}

    SyntaxBlockRef(PackedSyntaxTree* pst,
                   AST::PackedBlock const& packedBlock)
      : SyntaxTreeRef(pst, packedBlock)
    {}

    SyntaxBlockRef(PackedSyntaxTree* pst,
                   AST::PackedSizedBlock const& packedSizedBlock)
      : SyntaxTreeRef(pst, packedSizedBlock)
    {}

    SyntaxBlockRef(SyntaxBlock const* stBlock)
      : SyntaxTreeRef(stBlock)
    {}

    uint32_t numStatements() const {
        return numStatements_;
    }

    SyntaxNodeRef statement(uint32_t idx) const {
        WH_ASSERT(idx < numStatements());
        return SyntaxNodeRef(pst_, astBlock().statement(idx).offset());
    }

    AST::PackedBlock astBlock() const {
        return AST::PackedBlock(pst_->data(), offset(), numStatements_);
    }
};


//
// A SyntaxTreeFragment combines the following things:
//
// A reference to an underlying PackedSyntaxTree.
// An offset into the data of the PackedSyntaxTree identifying the
// subtree represented by the fragment.
//
class SyntaxTreeFragment
{
    friend struct TraceTraits<SyntaxTreeFragment>;

  protected:
    HeapField<PackedSyntaxTree*> pst_;
    uint32_t offset_;

    SyntaxTreeFragment(PackedSyntaxTree* pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst != nullptr);
    }

  public:
    PackedSyntaxTree* pst() const {
        return pst_;
    }

    uint32_t offset() const {
        return offset_;
    }

    bool isNode() const {
        return HeapThing::From(this)->isSyntaxNode();
    }
    SyntaxNode const* toNode() const {
        WH_ASSERT(isNode());
        return reinterpret_cast<SyntaxNode const*>(this);
    }
    SyntaxNode* toNode() {
        WH_ASSERT(isNode());
        return reinterpret_cast<SyntaxNode*>(this);
    }

    bool isBlock() const {
        return HeapThing::From(this)->isSyntaxBlock();
    }
    SyntaxBlock const* toBlock() const {
        WH_ASSERT(isBlock());
        return reinterpret_cast<SyntaxBlock const*>(this);
    }
    SyntaxBlock* toBlock() {
        WH_ASSERT(isBlock());
        return reinterpret_cast<SyntaxBlock*>(this);
    }
};

class SyntaxNode : public SyntaxTreeFragment
{
    friend struct TraceTraits<SyntaxNode>;

  public:
    SyntaxNode(PackedSyntaxTree* pst, uint32_t offset)
      : SyntaxTreeFragment(pst, offset)
    {}

    static Result<SyntaxNode*> Create(
            AllocationContext acx,
            Handle<PackedSyntaxTree*> pst,
            uint32_t offset);

    static Result<SyntaxNode*> Create(
            AllocationContext acx,
            Handle<SyntaxNodeRef> ref)
    {
        return Create(acx, ref->pst(), ref->offset());
    }

    AST::NodeType nodeType() const;
    char const* nodeTypeCString() const;

#define VM_STREF_GET_(ntype) \
    AST::Packed##ntype##Node ast##ntype() { \
        WH_ASSERT(nodeType() == AST::NodeType::ntype); \
        return SyntaxNodeRef(this).ast##ntype(); \
    }
    WHISPER_DEFN_SYNTAX_NODES(VM_STREF_GET_)
#undef VM_STREF_GET_
};

class SyntaxBlock : public SyntaxTreeFragment
{
    friend struct TraceTraits<SyntaxBlock>;
  protected:
    uint32_t numStatements_;

  public:
    SyntaxBlock(PackedSyntaxTree* pst, uint32_t offset, uint32_t numStatements)
      : SyntaxTreeFragment(pst, offset),
        numStatements_(numStatements)
    {}

    static Result<SyntaxBlock*> Create(
            AllocationContext acx,
            Handle<PackedSyntaxTree*> pst,
            uint32_t offset,
            uint32_t numStatements);

    static Result<SyntaxBlock*> Create(
            AllocationContext acx,
            Handle<SyntaxBlockRef> ref)
    {
        return Create(acx, ref->pst(), ref->offset(), ref->numStatements());
    }

    uint32_t numStatements() const {
        return numStatements_;
    }
};


} // namespace VM


//
// GC-Specializations.
//

template <>
struct TraceTraits<VM::PackedSyntaxTree>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::PackedSyntaxTree const& packedSt,
                     void const* start, void const* end)
    {
        packedSt.data_.scan(scanner, start, end);
        packedSt.constants_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::PackedSyntaxTree& packedSt,
                       void const* start, void const* end)
    {
        packedSt.data_.update(updater, start, end);
        packedSt.constants_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxTreeRef>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxTreeRef const& stRef,
                     void const* start, void const* end)
    {
        stRef.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxTreeRef& stRef,
                       void const* start, void const* end)
    {
        stRef.pst_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxNodeRef>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxNodeRef const& stRef,
                     void const* start, void const* end)
    {
        stRef.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxNodeRef& stRef,
                       void const* start, void const* end)
    {
        stRef.pst_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxBlockRef>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxBlockRef const& stRef,
                     void const* start, void const* end)
    {
        stRef.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxBlockRef& stRef,
                       void const* start, void const* end)
    {
        stRef.pst_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxTreeFragment>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxTreeFragment const& stFrag,
                     void const* start, void const* end)
    {
        stFrag.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxTreeFragment& stFrag,
                       void const* start, void const* end)
    {
        stFrag.pst_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxNode>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxNode const& stFrag,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxTreeFragment>::Scan<Scanner>(
            scanner, stFrag, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxNode& stFrag,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxTreeFragment>::Update<Updater>(
            updater, stFrag, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxBlock>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxBlock const& stFrag,
                     void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxTreeFragment>::Scan<Scanner>(
            scanner, stFrag, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxBlock& stFrag,
                       void const* start, void const* end)
    {
        TraceTraits<VM::SyntaxTreeFragment>::Update<Updater>(
            updater, stFrag, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PACKED_SYNTAX_TREE_HPP

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
// A SyntaxNodeRef is a on-stack reference to a node within
// a packed syntax tree.
//
class SyntaxNodeRef
{
    friend struct TraceTraits<SyntaxNodeRef>;

  protected:
    StackField<PackedSyntaxTree*> pst_;
    uint32_t offset_;

  public:
    SyntaxNodeRef()
      : pst_(nullptr),
        offset_(0)
    {}

    SyntaxNodeRef(PackedSyntaxTree* pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst_.get() != nullptr);
    }

    SyntaxNodeRef(PackedSyntaxTree* pst, AST::PackedBaseNode const& node)
      : SyntaxNodeRef(pst, node.offset())
    {}

    SyntaxNodeRef(SyntaxNode const* stNode);

    bool isValid() const {
        return pst_.get() != nullptr;
    }

    Handle<PackedSyntaxTree*> pst() const {
        WH_ASSERT(isValid());
        return pst_;
    }
    uint32_t offset() const {
        WH_ASSERT(isValid());
        return offset_;
    }

    AST::NodeType nodeType() const;
    char const* nodeTypeCString() const;

    AST::PackedBaseNode astBaseNode() {
        WH_ASSERT(isValid());
        return pst_->astBaseNodeAt(offset());
    }

#define VM_STREF_GET_(ntype) \
    bool is##ntype() const { \
        return nodeType() == AST::NodeType::ntype; \
    } \
    AST::Packed##ntype##Node ast##ntype() { \
        return astBaseNode().as##ntype(); \
    }
    WHISPER_DEFN_SYNTAX_NODES(VM_STREF_GET_)
#undef VM_STREF_GET_

    // Helper to create a new SyntaxNode for this one.
    Result<SyntaxNode*> createSyntaxNode(AllocationContext acx);
};


//
// A SyntaxNode is a heap-allocated variant of SyntaxNodeRef
//
class SyntaxNode
{
    friend struct TraceTraits<SyntaxNode>;

  protected:
    HeapField<PackedSyntaxTree*> pst_;
    uint32_t offset_;

  public:
    SyntaxNode(PackedSyntaxTree* pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst != nullptr);
    }

    PackedSyntaxTree* pst() const {
        return pst_;
    }

    uint32_t offset() const {
        return offset_;
    }

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
    bool is##ntype() const { \
        return nodeType() == AST::NodeType::ntype; \
    } \
    AST::Packed##ntype##Node ast##ntype() { \
        WH_ASSERT(is##ntype()); \
        return SyntaxNodeRef(this).ast##ntype(); \
    }
    WHISPER_DEFN_SYNTAX_NODES(VM_STREF_GET_)
#undef VM_STREF_GET_
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
struct TraceTraits<VM::SyntaxNodeRef>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxNodeRef const& snRef,
                     void const* start, void const* end)
    {
        snRef.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxNodeRef& snRef,
                       void const* start, void const* end)
    {
        snRef.pst_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::SyntaxNode>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SyntaxNode const& sn,
                     void const* start, void const* end)
    {
        sn.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SyntaxNode& sn,
                       void const* start, void const* end)
    {
        sn.pst_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PACKED_SYNTAX_TREE_HPP

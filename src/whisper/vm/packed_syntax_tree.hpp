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

    SyntaxTreeRef()
      : pst_(nullptr),
        offset_(0)
    {}

    SyntaxTreeRef(PackedSyntaxTree* pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst_.get() != nullptr);
    }

  public:
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

    SyntaxNodeRef(PackedSyntaxTree* pst, uint32_t offset)
      : SyntaxTreeRef(pst, offset)
    {
        WH_ASSERT(pst_.get() != nullptr);
    }

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
};

//
// A SyntaxBlockRef is a on-stack reference to a block or
// sized block in a packed syntax tree.
//
class SyntaxBlockRef : public SyntaxTreeRef
{
    friend struct TraceTraits<SyntaxBlockRef>;
  private:
    uint32_t numStatements_;

  public:
    SyntaxBlockRef() : SyntaxTreeRef() {}

    SyntaxBlockRef(PackedSyntaxTree* pst, uint32_t offset,
                   uint32_t numStatements)
      : SyntaxTreeRef(pst, offset),
        numStatements_(numStatements)
    {}

    SyntaxBlockRef(PackedSyntaxTree *pst, const AST::PackedBlock &packedBlock)
      : SyntaxBlockRef(pst, packedBlock.offset(), packedBlock.numStatements())
    {}

    SyntaxBlockRef(PackedSyntaxTree *pst,
                   const AST::PackedSizedBlock &packedSizedBlock)
      : SyntaxBlockRef(pst, packedSizedBlock.unsizedBlock())
    {}

    uint32_t numStatements() const {
        return numStatements_;
    }

    SyntaxNodeRef statement(uint32_t idx) const {
        WH_ASSERT(idx < numStatements());
        return SyntaxNodeRef(pst_, packedBlock().statement(idx).offset());
    }

  private:
    AST::PackedBlock packedBlock() const {
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

  private:
    HeapField<PackedSyntaxTree*> pst_;
    uint32_t offset_;

  public:
    SyntaxTreeFragment(PackedSyntaxTree* pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst != nullptr);
    }

    static Result<SyntaxTreeFragment*> Create(
            AllocationContext acx,
            Handle<PackedSyntaxTree*> pst,
            uint32_t offset);

    static Result<SyntaxTreeFragment*> Create(
            AllocationContext acx,
            Handle<SyntaxNodeRef> ref)
    {
        return Create(acx, ref->pst(), ref->offset());
    }

    PackedSyntaxTree* pst() const {
        return pst_;
    }

    uint32_t offset() const {
        return offset_;
    }

    AST::NodeType nodeType() const;
    char const* nodeTypeCString() const;
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


} // namespace Whisper


#endif // WHISPER__VM__PACKED_SYNTAX_TREE_HPP

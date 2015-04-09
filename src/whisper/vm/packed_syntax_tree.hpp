#ifndef WHISPER__VM__PACKED_SYNTAX_TREE_HPP
#define WHISPER__VM__PACKED_SYNTAX_TREE_HPP

#include "parser/syntax_tree.hpp"
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
    HeapField<Array<uint32_t> *> data_;
    HeapField<Array<Box> *> constants_;

  public:
    PackedSyntaxTree(Handle<Array<uint32_t> *> data,
                     Handle<Array<Box> *> constants)
      : data_(data),
        constants_(constants)
    {
        WH_ASSERT(data.get() != nullptr);
        WH_ASSERT(constants.get() != nullptr);
    }

    static Result<PackedSyntaxTree *> Create(AllocationContext acx,
                                             ArrayHandle<uint32_t> data,
                                             ArrayHandle<Box> constPool);

    Array<uint32_t> *data() const {
        return data_;
    }

    Array<Box> *constants() const {
        return constants_;
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
    HeapField<PackedSyntaxTree *> pst_;
    uint32_t offset_;

  public:
    SyntaxTreeFragment(Handle<PackedSyntaxTree *> pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst.get() != nullptr);
    }

    static Result<SyntaxTreeFragment *> Create(
            AllocationContext acx,
            Handle<PackedSyntaxTree *> pst,
            uint32_t offset);

    PackedSyntaxTree *pst() const {
        return pst_;
    }

    uint32_t offset() const {
        return offset_;
    }

    AST::NodeType nodeType() const;
    const char *nodeTypeCString() const;
};

//
// A SyntaxTreeRef is a on-stack version of SyntaxTreeFragment.
//
//
class SyntaxTreeRef
{
    friend struct TraceTraits<SyntaxTreeRef>;

  private:
    StackField<PackedSyntaxTree *> pst_;
    uint32_t offset_;

  public:
    SyntaxTreeRef(Handle<PackedSyntaxTree *> pst, uint32_t offset)
      : pst_(pst),
        offset_(offset)
    {
        WH_ASSERT(pst.get() != nullptr);
    }

    PackedSyntaxTree *pst() const {
        return pst_;
    }

    uint32_t offset() const {
        return offset_;
    }

    AST::NodeType nodeType() const;
    const char *nodeTypeCString() const;
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
    static void Scan(Scanner &scanner, const VM::PackedSyntaxTree &packedSt,
                     const void *start, const void *end)
    {
        packedSt.data_.scan(scanner, start, end);
        packedSt.constants_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::PackedSyntaxTree &packedSt,
                       const void *start, const void *end)
    {
        packedSt.data_.update(updater, start, end);
        packedSt.constants_.update(updater, start, end);
    }
};


template <>
struct TraceTraits<VM::SyntaxTreeFragment>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::SyntaxTreeFragment &stFrag,
                     const void *start, const void *end)
    {
        stFrag.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::SyntaxTreeFragment &stFrag,
                       const void *start, const void *end)
    {
        stFrag.pst_.update(updater, start, end);
    }
};


template <>
struct TraceTraits<VM::SyntaxTreeRef>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::SyntaxTreeRef &stRef,
                     const void *start, const void *end)
    {
        stRef.pst_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::SyntaxTreeRef &stRef,
                       const void *start, const void *end)
    {
        stRef.pst_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PACKED_SYNTAX_TREE_HPP

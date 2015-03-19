#ifndef WHISPER__VM__PACKED_SYNTAX_TREE_HPP
#define WHISPER__VM__PACKED_SYNTAX_TREE_HPP

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


} // namespace VM


//
// GC-Specializations for PackedSyntaxTree
//

template <>
struct HeapFormatTraits<HeapFormat::PackedSyntaxTree>
{
    HeapFormatTraits() = delete;
    typedef VM::PackedSyntaxTree Type;
};


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


} // namespace Whisper


#endif // WHISPER__VM__PACKED_SYNTAX_TREE_HPP

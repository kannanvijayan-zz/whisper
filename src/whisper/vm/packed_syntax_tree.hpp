#ifndef WHISPER__VM__PACKED_SYNTAX_TREE_HPP
#define WHISPER__VM__PACKED_SYNTAX_TREE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"
#include "vm/box.hpp"

#include <new>

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
    PackedSyntaxTree(Array<uint32_t> *data, Array<Box> *constants)
      : data_(data),
        constants_(constants)
    {
        WH_ASSERT(data != nullptr);
        WH_ASSERT(constants != nullptr);
    }

    static PackedSyntaxTree *Create(AllocationContext acx,
                                    uint32_t dataSize,
                                    const uint32_t *data,
                                    uint32_t constPoolSize,
                                    const Box *constPool);

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
struct HeapTraits<VM::PackedSyntaxTree>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = HeapFormat::PackedSyntaxTree;
    static constexpr bool VarSized = false;
};


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

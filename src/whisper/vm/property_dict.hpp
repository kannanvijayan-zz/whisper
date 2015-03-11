#ifndef WHISPER__VM__PROPERTY_DICT_HPP
#define WHISPER__VM__PROPERTY_DICT_HPP

#include "vm/core.hpp"
#include "vm/box.hpp"
#include "vm/string.hpp"

namespace Whisper {
namespace VM {


class PropertyDict
{
  friend class GC::TraceTraits<PropertyDict>;
  private:
    struct Entry
    {
        HeapField<String *> name;
        HeapField<Box> value;
    };

    uint32_t capacity_;
    uint32_t size_;
    Entry entries_[0];

  public:
    PropertyDict(uint32_t capacity)
      : capacity_(capacity),
        size_(0)
    {}

    uint32_t capacity() const {
        return capacity_;
    }
    uint32_t size() const {
        return size_;
    }
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct HeapTraits<VM::PropertyDict>
    {
        HeapTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::PropertyDict;
        static constexpr bool VarSized = true;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::PropertyDict>
    {
        AllocFormatTraits() = delete;
        typedef VM::PropertyDict Type;
    };

    template <>
    struct TraceTraits<VM::PropertyDict>
    {
        TraceTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr bool IsLeaf = false;

        template <typename Scanner>
        static void Scan(Scanner &scanner, const VM::PropertyDict &pd,
                         const void *start, const void *end)
        {
            for (uint32_t i = 0; i < pd.size_; i++) {
                pd.entries_[i].name.scan(scanner, start, end);
                pd.entries_[i].value.scan(scanner, start, end);
            }
        }

        template <typename Updater>
        static void Update(Updater &updater, VM::PropertyDict &pd,
                           const void *start, const void *end)
        {
            for (uint32_t i = 0; i < pd.size_; i++) {
                pd.entries_[i].name.update(updater, start, end);
                pd.entries_[i].value.update(updater, start, end);
            }
        }
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__PROPERTY_DICT_HPP

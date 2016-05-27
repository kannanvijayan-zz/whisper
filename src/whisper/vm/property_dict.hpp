#ifndef WHISPER__VM__PROPERTY_DICT_HPP
#define WHISPER__VM__PROPERTY_DICT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/string.hpp"
#include "vm/properties.hpp"

namespace Whisper {
namespace VM {


class PropertyDict
{
  friend class TraceTraits<PropertyDict>;
  private:
    static inline String* SENTINEL() {
        return reinterpret_cast<String*>(0x1u);
    }

    struct Entry
    {
        typedef Bitfield<uint8_t, uint8_t, 1, 0> KindBitfield;
        typedef Bitfield<uint8_t, uint8_t, 1, 1> SlotIsWritableBitfield;

        static constexpr uint8_t SLOT_KIND = 0;
        static constexpr uint8_t METHOD_KIND = 1;

        HeapField<String*> name;
        HeapField<Box> value;
        uint8_t flags_;

        PropertyDescriptor descriptor() const;
        void init(String* name, PropertyDescriptor const& descr,
                  PropertyDict* holderDict);
        void initDescriptor(PropertyDescriptor const& descr,
                            PropertyDict* holderDict);
    };

    uint32_t capacity_;
    uint32_t size_;
    Entry entries_[0];

    static constexpr float MaxFillRatio = 0.75;

  public:
    PropertyDict(uint32_t capacity)
      : capacity_(capacity),
        size_(0)
    {
        for (uint32_t i = 0; i < capacity_; i++)
            entries_[i].name.init(nullptr, this);
    }

    static uint32_t CalculateSize(uint32_t capacity) {
        return sizeof(PropertyDict) + (sizeof(Entry) * capacity);
    }

    static Result<PropertyDict*> Create(AllocationContext acx,
                                         uint32_t capacity);

    static Result<PropertyDict*> CreateEnlarged(AllocationContext acx,
                                                Handle<PropertyDict*> propDict);

    uint32_t capacity() const {
        return capacity_;
    }
    uint32_t size() const {
        return size_;
    }

    static uint32_t NameHash(String const* name);
    Maybe<uint32_t> lookup(String const* name) const;

    bool isValidEntry(uint32_t idx) const {
        WH_ASSERT(idx < capacity());
        String *name = entries_[idx].name.get();
        return (name != nullptr) && (name != SENTINEL());
    }

    String* name(uint32_t idx) const {
        WH_ASSERT(isValidEntry(idx));
        return entries_[idx].name;
    }
    PropertyDescriptor descriptor(uint32_t idx) const;

    void setDescriptor(uint32_t idx, PropertyDescriptor const& descr) {
        WH_ASSERT(idx < size());
        Box box;
        if (descr.isSlot()) {
            box = descr.slotValue();
        } else {
            WH_ASSERT(descr.isMethod());
            box = Box::Pointer(descr.methodFunction());
        }
        entries_[idx].value.set(box, this);
    }

    bool canAddEntry() const {
        return capacity_ * MaxFillRatio > size_;
    }
    Maybe<uint32_t> addEntry(String* name, PropertyDescriptor const& descr);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::PropertyDict>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::PropertyDict const& pd,
                     void const* start, void const* end)
    {
        for (uint32_t i = 0; i < pd.capacity_; i++) {
            if (!pd.isValidEntry(i))
                continue;
            pd.entries_[i].name.scan(scanner, start, end);
            pd.entries_[i].value.scan(scanner, start, end);
        }
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::PropertyDict& pd,
                       void const* start, void const* end)
    {
        for (uint32_t i = 0; i < pd.capacity_; i++) {
            if (!pd.isValidEntry(i))
                continue;
            pd.entries_[i].name.update(updater, start, end);
            pd.entries_[i].value.update(updater, start, end);
        }
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PROPERTY_DICT_HPP

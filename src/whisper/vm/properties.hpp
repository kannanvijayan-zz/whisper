#ifndef WHISPER__VM__PROPERTIES_HPP
#define WHISPER__VM__PROPERTIES_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/box.hpp"

#include <cstring>

namespace Whisper {
namespace VM {

class PropertyName
{
  friend class TraceTraits<PropertyName>;
  private:
    // Low bits of |val_| determine whether it is a VM::String pointer
    // or a raw char pointer.
    uintptr_t val_;
    uint32_t length_;

  public:
    PropertyName(String* vmString)
      : val_(reinterpret_cast<uintptr_t>(vmString) | 0x1u),
        length_(vmString->length())
    {
        WH_ASSERT(vmString != nullptr);
    }
    PropertyName(char const* cString, uint32_t length)
      : val_(reinterpret_cast<uintptr_t>(cString)),
        length_(length)
    {
        WH_ASSERT(cString != nullptr);
    }
    PropertyName(char const* cString)
      : val_(reinterpret_cast<uintptr_t>(cString)),
        length_(strlen(cString))
    {
        WH_ASSERT(cString != nullptr);
    }

    bool isVMString() const {
        return (val_ & 1) == 1;
    }
    bool isCString() const {
        return (val_ & 1) == 1;
    }
    String* vmString() const {
        WH_ASSERT(isVMString());
        return reinterpret_cast<String*>(val_ ^ 1u);
    }
    char const* cString() const {
        WH_ASSERT(isCString());
        return reinterpret_cast<char const*>(val_);
    }

    uint32_t length() const {
        return length_;
    }

    bool equals(String* str) const {
        if (isVMString())
            return vmString()->equals(str);

        WH_ASSERT(isCString());
        return str->equals(cString(), length());
    }

    Result<String*> createString(AllocationContext acx) const {
        if (isVMString())
            return OkVal(vmString());
        return String::Create(acx, length(), cString());
    }

  private:
    void gcUpdateVMString(String* str) {
        WH_ASSERT(str->length() == length_);
        val_ = reinterpret_cast<uintptr_t>(str) | 0x1u;
    }
};

class PropertySlotInfo
{
  private:
    bool isWritable_;

  public:
    PropertySlotInfo() : isWritable_(false) {}

    bool isWritable() const {
        return isWritable_;
    }
    void setWritable(bool writable) {
        isWritable_ = true;
    }
    PropertySlotInfo& withWritable(bool writable) {
        setWritable(writable);
        return *this;
    }
};

class PropertyDescriptor
{
  friend class TraceTraits<PropertyDescriptor>;
  private:
    enum class Kind : uint8_t {
        Invalid,
        Slot,
        Method
    };

    typedef Bitfield<uint8_t, uint8_t, 1, 0> SlotIsWritableBitfield;

    StackField<Box> value_;
    Kind kind_;
    uint8_t flags_;

    PropertyDescriptor(Kind kind, Box const& value)
      : value_(value), kind_(kind), flags_(0)
    {}

    PropertyDescriptor(Kind kind, Box const& value, uint8_t flags)
      : value_(value), kind_(kind), flags_(flags)
    {}

    static uint8_t SlotInfoToFlags(PropertySlotInfo info) {
        uint8_t flags = 0;
        SlotIsWritableBitfield(flags).initValue(info.isWritable());
        return flags;
    }
    static PropertySlotInfo FlagsToSlotInfo(uint8_t flags) {
        PropertySlotInfo result;
        result.setWritable(SlotIsWritableBitfield(flags).value());
        return result;
    }

  public:
    PropertyDescriptor()
      : value_(), kind_(Kind::Invalid), flags_(0)
    {}

    static PropertyDescriptor MakeSlot(ValBox const& value) {
        return PropertyDescriptor(Kind::Slot, value,
                                    SlotInfoToFlags(PropertySlotInfo()));
    }
    static PropertyDescriptor MakeSlot(ValBox const& value,
                                       PropertySlotInfo slotInfo)
    {
        return PropertyDescriptor(Kind::Slot, value,
                                  SlotInfoToFlags(slotInfo));
    }
    static PropertyDescriptor MakeMethod(Function* func) {
        return PropertyDescriptor(Kind::Method, Box::Pointer(func));
    }

    bool isValid() const {
        return kind_ != Kind::Invalid;
    }
    bool isSlot() const {
        return kind_ == Kind::Slot;
    }
    bool isMethod() const {
        return kind_ == Kind::Method;
    }

    ValBox slotValue() const {
        WH_ASSERT(isSlot());
        return ValBox(value_);
    }
    PropertySlotInfo slotInfo() const {
        WH_ASSERT(isSlot());
        return FlagsToSlotInfo(flags_);
    }
    Function* methodFunction() const {
        WH_ASSERT(isMethod());
        return value_->pointer<Function>();
    }
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::PropertyName>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::PropertyName const& propName,
                     void const* start, void const* end)
    {
        if (!propName.isVMString())
            return;
        scanner(&propName.val_, HeapThing::From(propName.vmString()));
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::PropertyName& propName,
                       void const* start, void const* end)
    {
        if (!propName.isVMString())
            return;
        HeapThing* old = HeapThing::From(propName.vmString());
        HeapThing* repl = updater(&propName.val_, old);
        if (repl != old) {
            VM::String* replStr = reinterpret_cast<VM::String*>(repl);
            propName.gcUpdateVMString(replStr);
        }
    }
};
template <>
struct TraceTraits<VM::PropertyDescriptor>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner,
                     VM::PropertyDescriptor const& propDesc,
                     void const* start, void const* end)
    {
        propDesc.value_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater,
                       VM::PropertyDescriptor& propDesc,
                       void const* start, void const* end)
    {
        propDesc.value_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__PROPERTIES_HPP

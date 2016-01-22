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
    // Low bits of pointer determine whether
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

class PropertyDescriptor
{
  friend class TraceTraits<PropertyDescriptor>;
  private:
    StackField<Box> value_;

  public:
    PropertyDescriptor()
      : value_()
    {}
    explicit PropertyDescriptor(Box const& value)
      : value_(value)
    {}
    explicit PropertyDescriptor(ValBox const& value)
      : value_(value)
    {}
    explicit PropertyDescriptor(Function* func)
      : value_(Box::Pointer(func))
    {}

    bool isValid() const;
    bool isValue() const;
    bool isMethod() const;

    Box const& box() const {
        WH_ASSERT(isValid());
        return value_;
    }
    Box const& value() const {
        WH_ASSERT(isValue());
        return value_;
    }
    Function* method() const {
        WH_ASSERT(isMethod());
        return value_->pointer<Function>();
    }

    ValBox valBox() const {
        WH_ASSERT(isValue());
        return ValBox(value_);
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

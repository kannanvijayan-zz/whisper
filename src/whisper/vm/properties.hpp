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
  friend class GC::TraceTraits<PropertyName>;
  private:
    // Low bits of pointer determine whether 
    uintptr_t val_;
    uint32_t length_;

  public:
    PropertyName(String *vmString)
      : val_(reinterpret_cast<uintptr_t>(vmString) | 0x1u),
        length_(vmString->length())
    {
        WH_ASSERT(vmString != nullptr);
    }
    PropertyName(const char *cString, uint32_t length)
      : val_(reinterpret_cast<uintptr_t>(cString)),
        length_(length)
    {
        WH_ASSERT(cString != nullptr);
    }
    PropertyName(const char *cString)
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
    String *vmString() const {
        WH_ASSERT(isVMString());
        return reinterpret_cast<String *>(val_ ^ 1u);
    }
    const char *cString() const {
        WH_ASSERT(isCString());
        return reinterpret_cast<const char *>(val_);
    }

    uint32_t length() const {
        return length_;
    }

    bool equals(String *str) const {
        if (isVMString())
            return vmString()->equals(str);

        WH_ASSERT(isCString());
        return str->equals(cString(), length());
    }

    String *createString(AllocationContext acx) const {
        if (isVMString())
            return vmString();
        return String::Create(acx, length(), cString());
    }

  private:
    void gcUpdateVMString(String *str) {
        WH_ASSERT(str->length() == length_);
        val_ = reinterpret_cast<uintptr_t>(str) | 0x1u;
    }
};

class PropertyDescriptor
{
  friend class GC::TraceTraits<PropertyDescriptor>;
  private:
    StackField<Box> value_;

  public:
    PropertyDescriptor(const Box &value)
      : value_(value)
    {}

    const Box &value() const {
        return value_;
    }
};


} // namespace VM
} // namespace Whisper


namespace Whisper {
namespace GC {

    template <>
    struct StackTraits<VM::PropertyName>
    {
        StackTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::PropertyName;
    };
    template <>
    struct StackTraits<VM::PropertyDescriptor>
    {
        StackTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr AllocFormat Format = AllocFormat::PropertyDescriptor;
    };

    template <>
    struct AllocFormatTraits<AllocFormat::PropertyName>
    {
        AllocFormatTraits() = delete;
        typedef VM::PropertyName Type;
    };
    template <>
    struct AllocFormatTraits<AllocFormat::PropertyDescriptor>
    {
        AllocFormatTraits() = delete;
        typedef VM::PropertyDescriptor Type;
    };

    template <>
    struct TraceTraits<VM::PropertyName>
    {
        TraceTraits() = delete;

        static constexpr bool Specialized = true;
        static constexpr bool IsLeaf = false;

        template <typename Scanner>
        static void Scan(Scanner &scanner, const VM::PropertyName &propName,
                         const void *start, const void *end)
        {
            if (!propName.isVMString())
                return;
            scanner(&propName.val_, AllocThing::From(propName.vmString()));
        }

        template <typename Updater>
        static void Update(Updater &updater, VM::PropertyName &propName,
                           const void *start, const void *end)
        {
            if (!propName.isVMString())
                return;
            AllocThing *old = AllocThing::From(propName.vmString());
            AllocThing *repl = updater(&propName.val_, old);
            if (repl != old) {
                VM::String *replStr = reinterpret_cast<VM::String *>(repl);
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
        static void Scan(Scanner &scanner,
                         const VM::PropertyDescriptor &propDesc,
                         const void *start, const void *end)
        {
            propDesc.value_.scan(scanner, start, end);
        }

        template <typename Updater>
        static void Update(Updater &updater,
                           VM::PropertyDescriptor &propDesc,
                           const void *start, const void *end)
        {
            propDesc.value_.update(updater, start, end);
        }
    };

} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__PROPERTIES_HPP

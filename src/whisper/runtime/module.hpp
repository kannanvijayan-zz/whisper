#ifndef WHISPER__RUNTIME__MODULE_HPP
#define WHISPER__RUNTIME__MODULE_HPP

#include "common.hpp"
#include "debug.hpp"

#include "runtime/rooting.hpp"
#include "runtime/heap_thing.hpp"
#include "runtime/string.hpp"

namespace Whisper {
namespace Runtime {


class ModuleVersion
{
  private:
    uint16_t major_;
    uint16_t minor_;
    uint16_t micro_;
    uint16_t nano_;

  public:
    ModuleVersion(uint16_t major, uint16_t minor, uint16_t micro, uint16_t nano)
      : major_(major), minor_(minor), micro_(micro), nano_(nano)
    {}

    inline uint16_t major() const {
        return major_;
    }

    inline uint16_t minor() const {
        return minor_;
    }

    inline uint16_t micro() const {
        return micro_;
    }

    inline uint16_t nano() const {
        return nano_;
    }
};


//
// Represents an entry in a module.  This just associates a name with
// a type and value.
//
class ModuleEntry : public HeapThing
{
  public:
    enum Type
    {
        FUNCTION
    };

  private:
    HeapPtr<String> name_;
    Type type_;
    HeapPtr<HeapThing> item_;

  public:
    ModuleEntry(HandlePtr<String> name, Type type, HandlePtr<HeapThing> item)
      : HeapThing(), name_(name), type_(type), item_(item)
    {}

    inline const HeapPtr<String> &name() const {
        return name_;
    }
    inline HeapPtr<String> &name() {
        return name_;
    }

    inline const Type type() const {
        return type_;
    }

    inline const HeapPtr<HeapThing> &item() const {
        return item_;
    }
    inline HeapPtr<HeapThing> &item() {
        return item_;
    }
};

//
// Object that represents a module.
//
class Module : public HeapThing
{
  private:
    // Location the module was loaded from.
    HeapPtr<String> location_;

    // Module name (maybe null).
    HeapPtr<String> name_;

    // Module version.
    ModuleVersion version_;

    // The entries in the module.
    uint32_t numEntries_;
    HeapPtrArray<ModuleEntry> entries_;

  public:
    Module(HandlePtr<String> location,
           HandlePtr<String> name,
           const ModuleVersion &version,
           uint32_t numEntries,
           HandlePtrArray<ModuleEntry> entries)
      : location_(location),
        name_(name),
        version_(version),
        numEntries_(numEntries),
        entries_(numEntries, entries)
    {}

    const HeapPtr<String> &location() const {
        return location_;
    }
    HeapPtr<String> &location() {
        return location_;
    }

    const HeapPtr<String> &name() const {
        return name_;
    }
    HeapPtr<String> &name() {
        return name_;
    }

    const ModuleVersion &version() const {
        return version_;
    }

    const uint32_t numEntries() const {
        return numEntries_;
    }

    const HeapPtrArray<ModuleEntry> &entries() const {
        return entries_;
    }
    HeapPtrArray<ModuleEntry> &entries() {
        return entries_;
    }
};

template <>
class HeapThingTraits<ModuleEntry>
{
    static constexpr HeapType HEAP_TYPE = HeapType::ModuleEntry;
    static constexpr bool TERMINAL = false;
};

template <>
class HeapThingTraits<Module>
{
    static constexpr HeapType HEAP_TYPE = HeapType::Module;
    static constexpr bool TERMINAL = false;
};


} // namespace Runtime
} // namespace Whisper

#endif // WHISPER__RUNTIME__MODULE_HPP

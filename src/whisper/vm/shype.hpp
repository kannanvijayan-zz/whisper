#ifndef WHISPER__VM__SHYPE_HPP
#define WHISPER__VM__SHYPE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"

#include <new>

namespace Whisper {
namespace VM {

    class Shype;
    class RootShype;
    class AddSlotShype;

} // namespace VM
} // namespace Whisper

#include "vm/shype_pre_specializations.hpp"

namespace Whisper {
namespace VM {

//
// Base class for shypes.
//
class Shype
{
    template <typename T>
    friend class GC::TraceTraits;

  protected:
    HeapField<Shype *> parent_;
    HeapField<Shype *> firstChild_;
    HeapField<Shype *> nextSibling_;

  protected:
    Shype() {}

  public:
    bool hasParent() const {
        return parent_ != nullptr;
    }
    Shype *parent() const {
        WH_ASSERT(hasParent());
        return parent_;
    }

    bool hasChild() const {
        return firstChild_ != nullptr;
    }
    Shype *firstChild() const {
        WH_ASSERT(hasChild());
        return firstChild_;
    }

    bool hasSibling() const {
        return nextSibling_ != nullptr;
    }
    Shype *nextSibling() const {
        WH_ASSERT(hasSibling());
        return nextSibling_;
    }
};

//
// Root shype.
//
class RootShype : public Shype
{
    friend class GC::TraceTraits<RootShype>;

  public:
    RootShype() : Shype() {}
};

//
// AddSlot shype.
//
class AddSlotShype : public Shype
{
    friend class GC::TraceTraits<AddSlotShype>;

  private:
    HeapField<String *> name_;
    uint32_t slotno_;

  public:
    AddSlotShype(String *name, uint32_t slotno)
      : Shype(),
        name_(name),
        slotno_(slotno)
    {}

    HeapField<String *> &name() {
        return name_;
    }
    const HeapField<String *> &name() const {
        return name_;
    }

    uint32_t slotno() const {
        return slotno_;
    }
};


} // namespace VM
} // namespace Whisper


#include "vm/shype_post_specializations.hpp"


#endif // WHISPER__VM__SHYPE_HPP

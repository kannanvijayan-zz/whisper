#ifndef WHISPER__VM__HASH_OBJECT_HPP
#define WHISPER__VM__HASH_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/wobject.hpp"
#include "vm/property_dict.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {


class HashObject : public Wobject
{
  friend class TraceTraits<HashObject>;
  protected:
    HeapField<Array<Wobject*>*> delegates_;
    HeapField<PropertyDict*> dict_;

    // Initial dictionary size is 8.
    static constexpr uint32_t InitialPropertyCapacity = 8;

  public:
    HashObject(Array<Wobject*>* delegates, PropertyDict* dict)
      : Wobject(),
        delegates_(delegates),
        dict_(dict)
    {
        WH_ASSERT(delegates_ != nullptr);
        WH_ASSERT(dict_ != nullptr);
    }

    WobjectHooks const* hashObjectHooks() const;

    static uint32_t NumDelegates(AllocationContext acx,
                                 Handle<HashObject*> obj);

    static void GetDelegates(AllocationContext acx,
                             Handle<HashObject*> obj,
                             MutHandle<Array<Wobject*>*> delegatesOut);

    static bool GetProperty(AllocationContext acx,
                            Handle<HashObject*> obj,
                            Handle<String*> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(AllocationContext acx,
                                   Handle<HashObject*> obj,
                                   Handle<String*> name,
                                   Handle<PropertyDescriptor> defn);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::HashObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::HashObject const& obj,
                     void const* start, void const* end)
    {
        obj.delegates_.scan(scanner, start, end);
        obj.dict_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::HashObject& obj,
                       void const* start, void const* end)
    {
        obj.delegates_.update(updater, start, end);
        obj.dict_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__HASH_OBJECT_HPP

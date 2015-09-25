#ifndef WHISPER__VM__RUNTIME_STATE_HPP
#define WHISPER__VM__RUNTIME_STATE_HPP


#include "name_pool.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {

//
// A RuntimeState contains mappings from symbols to the location within the
// file that contains the symbol definition.
//
class RuntimeState
{
    friend struct TraceTraits<RuntimeState>;

  private:
    HeapField<Array<String*>*> namePool_;

  public:
    RuntimeState(Array<String*>* namePool)
      : namePool_(namePool)
    {
        WH_ASSERT(namePool != nullptr);
    }

    static Result<RuntimeState*> Create(AllocationContext acx);

#define NAME_METH_(name, str) \
    String* nm_##name() const { \
        return namePool_->get(NamePool::IndexOfId(NamePool::Id::name)); \
    }
    WHISPER_DEFN_NAME_POOL(NAME_METH_)
#undef NAME_METH_
};


} // namespace VM

//
// GC Specializations
//

template <>
struct TraceTraits<VM::RuntimeState>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::RuntimeState const& rtState,
                     void const* start, void const* end)
    {
        rtState.namePool_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::RuntimeState& rtState,
                       void const* start, void const* end)
    {
        rtState.namePool_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__RUNTIME_STATE_HPP

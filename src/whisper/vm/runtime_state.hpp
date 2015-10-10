#ifndef WHISPER__VM__RUNTIME_STATE_HPP
#define WHISPER__VM__RUNTIME_STATE_HPP


#include "name_pool.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {

class GlobalScope;
class Wobject;

//
// A RuntimeState is a vm-allocated object that holds pointers to
// universal, runtime-related objects.
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

//
// A ThreadState is a vm-allocated object that holds pointer to global,
// thread-related objects, such as the global, root delegate, etc.
//
class ThreadState
{
    friend struct TraceTraits<ThreadState>;

  private:
    HeapField<GlobalScope*> global_;
    HeapField<Wobject*> rootDelegate_;
    HeapField<Wobject*> immIntDelegate_;
    HeapField<Wobject*> immBoolDelegate_;

    // If an error occurs during execution, it is recorded
    // here.
    RuntimeError error_;
    char const* errorString_;
    HeapField<HeapThing*> errorThing_;

  public:
    ThreadState(GlobalScope* global,
                Wobject* rootDelegate,
                Wobject* immIntDelegate,
                Wobject* immBoolDelegate)
      : global_(global),
        rootDelegate_(rootDelegate),
        immIntDelegate_(immIntDelegate),
        immBoolDelegate_(immBoolDelegate)
    {
        WH_ASSERT(global != nullptr);
        WH_ASSERT(rootDelegate != nullptr);
        WH_ASSERT(immIntDelegate != nullptr);
        WH_ASSERT(immBoolDelegate != nullptr);
    }

    GlobalScope* global() const {
        return global_;
    }
    Wobject* rootDelegate() const {
        return rootDelegate_;
    }
    Wobject* immIntDelegate() const {
        return immIntDelegate_;
    }
    Wobject* immBoolDelegate() const {
        return immBoolDelegate_;
    }

    bool hasError() const {
        return error_ == RuntimeError::None;
    }
    RuntimeError error() const {
        WH_ASSERT(hasError());
        return error_;
    }

    bool hasErrorString() const {
        WH_ASSERT(hasError());
        return errorString_ != nullptr;
    }
    char const* errorString() const {
        WH_ASSERT(hasErrorString());
        return errorString_;
    }

    bool hasErrorThing() const {
        WH_ASSERT(hasError());
        return errorThing_ != nullptr;
    }
    HeapThing* errorThing() const {
        WH_ASSERT(hasErrorThing());
        return errorThing_;
    }

    void setError(RuntimeError error, char const* string, HeapThing* thing)
    {
        WH_ASSERT(!hasError());
        error_ = error;
        errorString_ = string;
        errorThing_.set(thing, this);
    }

    static Result<ThreadState*> Create(AllocationContext acx);
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

template <>
struct TraceTraits<VM::ThreadState>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::ThreadState const& rtState,
                     void const* start, void const* end)
    {
        rtState.global_.scan(scanner, start, end);
        rtState.rootDelegate_.scan(scanner, start, end);
        rtState.errorThing_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::ThreadState& rtState,
                       void const* start, void const* end)
    {
        rtState.global_.update(updater, start, end);
        rtState.rootDelegate_.update(updater, start, end);
        rtState.errorThing_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__RUNTIME_STATE_HPP

#ifndef WHISPER__VM__SCOPE_OBJECT_HPP
#define WHISPER__VM__SCOPE_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


class ScopeObject : public HashObject
{
  friend class TraceTraits<ScopeObject>;
  public:
    ScopeObject(Handle<Array<Wobject *> *> delegates,
                Handle<PropertyDict *> dict)
      : HashObject(delegates, dict)
    {}
};

class CallScope : public ScopeObject
{
  friend class TraceTraits<CallScope>;
  public:
    CallScope(Handle<Array<Wobject *> *> delegates,
              Handle<PropertyDict *> dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<CallScope *> Create(AllocationContext acx,
                                      Handle<ScopeObject *> callerScope);

    static void GetDelegates(AllocationContext acx,
                             Handle<CallScope *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetProperty(AllocationContext acx,
                            Handle<CallScope *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(AllocationContext acx,
                                   Handle<CallScope *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);
};

class ModuleScope : public ScopeObject
{
  friend class TraceTraits<ModuleScope>;
  public:
    ModuleScope(Handle<Array<Wobject *> *> delegates,
                Handle<PropertyDict *> dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<ModuleScope *> Create(AllocationContext acx,
                                        Handle<GlobalScope *> global);

    static void GetDelegates(AllocationContext acx,
                             Handle<ModuleScope *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetProperty(AllocationContext acx,
                            Handle<ModuleScope *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(AllocationContext acx,
                                   Handle<ModuleScope *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::CallScope>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::CallScope &scope,
                     const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, scope, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::CallScope &scope,
                       const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Update(updater, scope, start, end);
    }
};

template <>
struct TraceTraits<VM::ModuleScope>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::ModuleScope &scope,
                     const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, scope, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::ModuleScope &scope,
                       const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Update(updater, scope, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SCOPE_OBJECT_HPP

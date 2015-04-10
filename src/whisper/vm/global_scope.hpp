#ifndef WHISPER__VM__GLOBAL_SCOPE_HPP
#define WHISPER__VM__GLOBAL_SCOPE_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
#include "vm/scope_object.hpp"

namespace Whisper {
namespace VM {


class GlobalScope : public ScopeObject
{
  friend class TraceTraits<GlobalScope>;
  public:
    GlobalScope(Handle<Array<Wobject *> *> delegates,
                Handle<PropertyDict *> dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<GlobalScope *> Create(AllocationContext acx);

    static void GetDelegates(ThreadContext *cx,
                             Handle<GlobalScope *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetPropertyIndex(Handle<GlobalScope *> obj,
                                 Handle<String *> name,
                                 uint32_t *indexOut);

    static bool GetProperty(ThreadContext *cx,
                            Handle<GlobalScope *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(ThreadContext *cx,
                                   Handle<GlobalScope *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);

  private:
    static OkResult BindSyntaxHandlers(AllocationContext acx,
                                       Handle<GlobalScope *> obj);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::GlobalScope>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::GlobalScope &scope,
                     const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, scope, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::GlobalScope &scope,
                       const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Update(updater, scope, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__GLOBAL_SCOPE_HPP
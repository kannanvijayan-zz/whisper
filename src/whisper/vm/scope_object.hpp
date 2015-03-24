#ifndef WHISPER__VM__SCOPE_OBJECT_HPP
#define WHISPER__VM__SCOPE_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


class CallObject;


class ScopeObject : public HashObject
{
  friend class TraceTraits<ScopeObject>;
  public:
    ScopeObject(Handle<Array<Wobject *> *> delegates,
                Handle<PropertyDict *> dict)
      : HashObject(delegates, dict)
    {}
};

class CallObject : public ScopeObject
{
  friend class TraceTraits<CallObject>;
  public:
    CallObject(Handle<Array<Wobject *> *> delegates,
               Handle<PropertyDict *> dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<CallObject *> Create(AllocationContext acx,
                                       Handle<ScopeObject *> callerScope);

    static void GetDelegates(ThreadContext *cx,
                             Handle<CallObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetPropertyIndex(Handle<CallObject *> obj,
                                 Handle<String *> name,
                                 uint32_t *indexOut);

    static bool GetProperty(ThreadContext *cx,
                            Handle<CallObject *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(ThreadContext *cx,
                                   Handle<CallObject *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);
};

class GlobalObject : public ScopeObject
{
  friend class TraceTraits<GlobalObject>;
  public:
    GlobalObject(Handle<Array<Wobject *> *> delegates,
                 Handle<PropertyDict *> dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<GlobalObject *> Create(AllocationContext acx);

    static void GetDelegates(ThreadContext *cx,
                             Handle<GlobalObject *> obj,
                             MutHandle<Array<Wobject *> *> delegatesOut);

    static bool GetPropertyIndex(Handle<GlobalObject *> obj,
                                 Handle<String *> name,
                                 uint32_t *indexOut);

    static bool GetProperty(ThreadContext *cx,
                            Handle<GlobalObject *> obj,
                            Handle<String *> name,
                            MutHandle<PropertyDescriptor> result);

    static OkResult DefineProperty(ThreadContext *cx,
                                   Handle<GlobalObject *> obj,
                                   Handle<String *> name,
                                   Handle<PropertyDescriptor> defn);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::CallObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::CallObject &obj,
                     const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::CallObject &obj,
                       const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Update(updater, obj, start, end);
    }
};

template <>
struct TraceTraits<VM::GlobalObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const VM::GlobalObject &obj,
                     const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, obj, start, end);
    }

    template <typename Updater>
    static void Update(Updater &updater, VM::GlobalObject &obj,
                       const void *start, const void *end)
    {
        TraceTraits<VM::HashObject>::Update(updater, obj, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SCOPE_OBJECT_HPP

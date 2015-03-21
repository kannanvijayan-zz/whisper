
#include "runtime_inlines.hpp"
#include "vm/scope_object.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<CallObject *>
CallObject::Create(AllocationContext acx,
                   Handle<ScopeObject *> callerScope)
{
    // Allocate array of delegates containing caller scope.
    Local<Array<Wobject *> *> delegates(acx);
    if (!delegates.setResult(Array<Wobject *>::Create(acx, 1,
            static_cast<Wobject*>(nullptr))))
    {
        return Result<CallObject *>::Error();
    }
    delegates->set(0, callerScope.get());

    // Allocate a dictionary.
    Local<PropertyDict *> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return Result<CallObject *>::Error();

    return acx.create<CallObject>(delegates.handle(), props.handle());
}

/* static */ void
CallObject::GetDelegates(ThreadContext *cx,
                         Handle<CallObject *> obj,
                         MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(cx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
CallObject::GetProperty(ThreadContext *cx,
                        Handle<CallObject *> obj,
                        Handle<String *> name,
                        MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
CallObject::DefineProperty(ThreadContext *cx,
                           Handle<CallObject *> obj,
                           Handle<String *> name,
                           Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}


/* static */ Result<GlobalObject *>
GlobalObject::Create(AllocationContext acx)
{
    // Allocate empty array of delegates.
    Local<Array<Wobject *> *> delegates(acx);
    if (!delegates.setResult(Array<Wobject *>::Create(acx, 1,
            static_cast<Wobject*>(nullptr))))
    {
        return Result<GlobalObject *>::Error();
    }

    // Allocate a dictionary.
    Local<PropertyDict *> props(acx);
    if (!props.setResult(PropertyDict::Create(acx, InitialPropertyCapacity)))
        return Result<GlobalObject *>::Error();

    return acx.create<GlobalObject>(delegates.handle(), props.handle());
}

/* static */ void
GlobalObject::GetDelegates(ThreadContext *cx,
                           Handle<GlobalObject *> obj,
                           MutHandle<Array<Wobject *> *> delegatesOut)
{
    HashObject::GetDelegates(cx,
        Handle<HashObject *>::Convert(obj),
        delegatesOut);
}

/* static */ bool
GlobalObject::GetProperty(ThreadContext *cx,
                          Handle<GlobalObject *> obj,
                          Handle<String *> name,
                          MutHandle<PropertyDescriptor> result)
{
    return HashObject::GetProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, result);
}

/* static */ OkResult
GlobalObject::DefineProperty(ThreadContext *cx,
                             Handle<GlobalObject *> obj,
                             Handle<String *> name,
                             Handle<PropertyDescriptor> defn)
{
    return HashObject::DefineProperty(cx,
        Handle<HashObject *>::Convert(obj),
        name, defn);
}


} // namespace VM
} // namespace Whisper

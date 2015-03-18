
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ OkResult
Wobject::GetDelegates(ThreadContext *cx,
                      Handle<Wobject *> obj,
                      MutHandle<Array<Wobject *> *> delegatesOut)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        PlainObject::GetDelegates(cx, plainObj, delegatesOut);
        return OkResult::Ok();
    }

    WH_UNREACHABLE("Unknown object kind");
    return OkResult::Error();
}

/* static */ Result<bool>
Wobject::LookupProperty(ThreadContext *cx,
                        Handle<Wobject *> obj,
                        Handle<PropertyName> name,
                        MutHandle<PropertyDescriptor> result)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        bool got = PlainObject::LookupProperty(cx, plainObj, name, result);
        return Result<bool>::Value(got);
    }

    WH_UNREACHABLE("Unknown object kind");
    return Result<bool>::Error();
}

/* static */ OkResult
Wobject::DefineProperty(ThreadContext *cx,
                        Handle<Wobject *> obj,
                        Handle<PropertyName> name,
                        Handle<PropertyDescriptor> defn)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        return PlainObject::DefineProperty(cx, plainObj, name, defn);
    }

    WH_UNREACHABLE("Unknown object kind");
    return OkResult::Error();
}


} // namespace VM
} // namespace Whisper

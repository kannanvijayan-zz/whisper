
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ bool
Wobject::GetDelegates(ThreadContext *cx,
                      Handle<Wobject *> obj,
                      MutHandle<Array<Wobject *> *> delegatesOut)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        return PlainObject::GetDelegates(cx, plainObj, delegatesOut);
    }

    WH_UNREACHABLE("Unknown object kind");
    return false;
}

/* static */ bool
Wobject::LookupProperty(ThreadContext *cx,
                        Handle<Wobject *> obj,
                        Handle<PropertyName> name,
                        MutHandle<PropertyDescriptor> result)
{
    HeapThing *heapThing = HeapThing::From(obj.get());
    if (heapThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(heapThing));
        return PlainObject::LookupProperty(cx, plainObj, name, result);
    }

    WH_UNREACHABLE("Unknown object kind");
    return false;
}

/* static */ bool
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
    return false;
}


} // namespace VM
} // namespace Whisper

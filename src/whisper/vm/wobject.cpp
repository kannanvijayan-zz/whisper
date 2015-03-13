
#include "vm/wobject.hpp"
#include "vm/plain_object.hpp"

namespace Whisper {
namespace VM {


/* static */ bool
Wobject::GetDelegates(RunContext *cx,
                      Handle<Wobject *> obj,
                      MutHandle<Array<Wobject *> *> delegatesOut)
{
    GC::AllocThing *allocThing = GC::AllocThing::From(obj.get());
    if (allocThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(allocThing));
        return PlainObject::GetDelegates(cx, plainObj, delegatesOut);
    }

    WH_UNREACHABLE("Unknown object kind");
    return false;
}

/* static */ bool
Wobject::LookupProperty(RunContext *cx,
                        Handle<Wobject *> obj,
                        Handle<PropertyName> name,
                        MutHandle<PropertyDescriptor> result)
{
    GC::AllocThing *allocThing = GC::AllocThing::From(obj.get());
    if (allocThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(allocThing));
        return PlainObject::LookupProperty(cx, plainObj, name, result);
    }

    WH_UNREACHABLE("Unknown object kind");
    return false;
}

/* static */ bool
Wobject::DefineProperty(RunContext *cx,
                        Handle<Wobject *> obj,
                        Handle<PropertyName> name,
                        Handle<PropertyDescriptor> defn)
{
    GC::AllocThing *allocThing = GC::AllocThing::From(obj.get());
    if (allocThing->isPlainObject()) {
        Local<PlainObject *> plainObj(cx,
            reinterpret_cast<PlainObject *>(allocThing));
        return PlainObject::DefineProperty(cx, plainObj, name, defn);
    }

    WH_UNREACHABLE("Unknown object kind");
    return false;
}


} // namespace VM
} // namespace Whisper

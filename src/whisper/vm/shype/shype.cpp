
#include "vm/core.hpp"
#include "vm/shype/shype.hpp"
#include "vm/shype/dict_shype.hpp"

namespace Whisper {
namespace VM {


bool
Shype::lookupProperty(RunContext *cx,
                      Handle<Wobject *> obj,
                      Handle<PropertyName> name,
                      MutHandle<PropertyDescriptor> result)
{
    switch (GC::AllocThing::From(this)->header().format()) {
      case GC::AllocFormat::DictShype: {
        DictShype *dictShype = reinterpret_cast<DictShype *>(this);
        return dictShype->lookupDictProperty(cx, obj, name, result);
      }
      default:
        WH_UNREACHABLE("Unknown shype format.");
    }
}

bool
Shype::defineProperty(RunContext *cx,
                      Handle<Wobject *> obj,
                      Handle<PropertyName> name,
                      Handle<PropertyDescriptor> defn)
{
    switch (GC::AllocThing::From(this)->header().format()) {
      case GC::AllocFormat::DictShype: {
        DictShype *dictShype = reinterpret_cast<DictShype *>(this);
        return dictShype->defineDictProperty(cx, obj, name, defn);
      }
      default:
        WH_UNREACHABLE("Unknown shype format.");
    }
}


} // namespace VM
} // namespace Whisper

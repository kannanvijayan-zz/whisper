
#include "vm/core.hpp"
#include "vm/shype/shype.hpp"
#include "vm/shype/dict_shype.hpp"

namespace Whisper {
namespace VM {


bool
Shype::lookupProperty(RunContext *cx,
                      Wobject *obj,
                      const PropertyName &name,
                      PropertyDescriptor *resultOut)
{
    switch (GC::AllocThing::From(this)->header().format()) {
      case GC::AllocFormat::DictShype: {
        DictShype *dictShype = reinterpret_cast<DictShype *>(this);
        return dictShype->lookupDictProperty(cx, obj, name, resultOut);
      }
      default:
        WH_UNREACHABLE("Unknown shype format.");
        return false;
    }
}

bool
Shype::defineProperty(RunContext *cx,
                      Wobject *obj,
                      const PropertyName &name,
                      const PropertyDescriptor &defn)
{
    switch (GC::AllocThing::From(this)->header().format()) {
      case GC::AllocFormat::DictShype: {
        DictShype *dictShype = reinterpret_cast<DictShype *>(this);
        return dictShype->defineDictProperty(cx, obj, name, defn);
      }
      default:
        WH_UNREACHABLE("Unknown shype format.");
        return false;
    }
}


} // namespace VM
} // namespace Whisper

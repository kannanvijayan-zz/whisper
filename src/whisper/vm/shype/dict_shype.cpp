
#include "vm/shype/dict_shype.hpp"
#include "vm/wobject.hpp"

namespace Whisper {
namespace VM {


bool
DictShype::lookupDictProperty(RunContext *cx,
                              Wobject *obj,
                              const PropertyName &name,
                              PropertyDescriptor *resultOut)
{
    return false;
}

bool
DictShype::defineDictProperty(RunContext *cx,
                              Wobject *obj,
                              const PropertyName &name,
                              const PropertyDescriptor &defn)
{
    return false;
}


} // namespace VM
} // namespace Whisper

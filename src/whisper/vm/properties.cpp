
#include "vm/core.hpp"
#include "vm/properties.hpp"
#include "vm/wobject.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


bool
PropertyDescriptor::isValue() const
{
    WH_ASSERT(!value_->isInvalid());
    if (!value_->isPointer())
        return true;

    return Wobject::IsWobject(value_->pointer<HeapThing>());
}

bool
PropertyDescriptor::isMethod() const
{
    WH_ASSERT(!value_->isInvalid());
    if (!value_->isPointer())
        return false;

    return Function::IsFunction(value_->pointer<HeapThing>());
}


} // namespace VM
} // namespace Whisper

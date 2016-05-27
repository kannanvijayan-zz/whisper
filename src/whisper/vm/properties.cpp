
#include "vm/core.hpp"
#include "vm/properties.hpp"
#include "vm/wobject.hpp"
#include "vm/function.hpp"

namespace Whisper {
namespace VM {


bool
PropertyDescriptor::isValid() const
{
    if (value_->isInvalid())
        return false;

    if (!value_->isPointer())
        return true;

    // Pointer-based value must be either a HeapThing
    // or a Function.
    if (Wobject::IsWobject(value_->pointer<HeapThing>()))
        return true;
    if (Function::IsFunction(value_->pointer<HeapThing>()))
        return true;
    return false;
}

bool
PropertyDescriptor::isSlot() const
{
    WH_ASSERT(isValid());
    if (!value_->isPointer())
        return true;

    return Wobject::IsWobject(value_->pointer<HeapThing>());
}

bool
PropertyDescriptor::isMethod() const
{
    WH_ASSERT(isValid());
    if (!value_->isPointer())
        return false;

    return Function::IsFunction(value_->pointer<HeapThing>());
}


} // namespace VM
} // namespace Whisper

#ifndef WHISPER__INTERP__OBJECT_BEHAVIOUR_HPP
#define WHISPER__INTERP__OBJECT_BEHAVIOUR_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/control_flow.hpp"

namespace Whisper {
namespace Interp {

Result<VM::Wobject*> CreateRootDelegate(AllocationContext acx);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__OBJECT_BEHAVIOUR_HPP

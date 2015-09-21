#ifndef WHISPER__INTERP__SYNTAX_BEHAVIOUR_HPP
#define WHISPER__INTERP__SYNTAX_BEHAVIOUR_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/control_flow.hpp"

namespace Whisper {
namespace Interp {

OkResult BindSyntaxHandlers(AllocationContext acx,
                            VM::GlobalScope *global);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__SYNTAX_BEHAVIOUR_HPP

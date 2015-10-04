#ifndef WHISPER__INTERP__OBJECT_BEHAVIOUR_HPP
#define WHISPER__INTERP__OBJECT_BEHAVIOUR_HPP

#include "vm/core.hpp"
#include "vm/wobject.hpp"

namespace Whisper {
namespace Interp {

Result<VM::Wobject*> CreateRootDelegate(AllocationContext acx);

Result<VM::Wobject*> CreateImmIntDelegate(AllocationContext acx,
                                          Handle<VM::Wobject*> rootDelegate);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__OBJECT_BEHAVIOUR_HPP

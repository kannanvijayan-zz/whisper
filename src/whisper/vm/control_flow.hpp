#ifndef WHISPER__VM__CONTROL_FLOW_HPP
#define WHISPER__VM__CONTROL_FLOW_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"

namespace Whisper {
namespace VM {


enum class ControlFlow
{
    Void,
    Value,
    Return
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__CONTROL_FLOW_HPP

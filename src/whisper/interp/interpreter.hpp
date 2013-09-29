#ifndef WHISPER__INTERP__INTERPRETER_HPP
#define WHISPER__INTERP__INTERPRETER_HPP

#include "common.hpp"
#include "runtime.hpp"
#include "vm/script.hpp"

namespace Whisper {
namespace Interp {


/**
 * Interpreter implementation.
 */
bool InterpretScript(RunContext *cx, VM::HandleScript script); 

} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__INTERPRETER_HPP

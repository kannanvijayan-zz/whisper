#ifndef WHISPER__INTERP__PROPERTY_LOOKUP_HPP
#define WHISPER__INTERP__PROPERTY_LOOKUP_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/control_flow.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace Interp {


VM::ControlFlow GetValueProperty(ThreadContext* cx,
                                 Handle<VM::ValBox> value,
                                 Handle<VM::String*> name);

VM::ControlFlow GetObjectProperty(ThreadContext* cx,
                                  Handle<VM::Wobject*> object,
                                  Handle<VM::String*> name);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__PROPERTY_LOOKUP_HPP

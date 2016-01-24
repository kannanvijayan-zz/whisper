#ifndef WHISPER__INTERP__HEAP_INTERPRETER_HPP
#define WHISPER__INTERP__HEAP_INTERPRETER_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/source_file.hpp"
#include "vm/scope_object.hpp"
#include "vm/properties.hpp"
#include "vm/lookup_state.hpp"
#include "vm/control_flow.hpp"
#include "vm/frame.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace Interp {

VM::ControlFlow HeapInterpretSourceFile(ThreadContext* cx,
                                        Handle<VM::SourceFile*> file,
                                        Handle<VM::ScopeObject*> scope);

VM::ControlFlow HeapInterpretLoop(ThreadContext* cx);

Result<VM::Frame*> CreateInitialSyntaxFrame(
        ThreadContext* cx,
        Handle<VM::EntryFrame*> entryFrame);

Result<VM::Frame*> CreateInvokeSyntaxFrame(
        ThreadContext* cx,
        Handle<VM::Frame*> parentFrame,
        Handle<VM::EntryFrame*> entryFrame,
        Handle<VM::SyntaxTreeFragment*> stFrag,
        Handle<VM::ValBox> syntaxHandler);

OkResult InvokeValue(
        ThreadContext* cx,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        ArrayHandle<VM::SyntaxTreeFragment*> args);

OkResult InvokeFunction(
        ThreadContext* cx,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::SyntaxTreeFragment*> args);

OkResult InvokeOperativeFunction(
        ThreadContext* cx,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::SyntaxTreeFragment*> args);

OkResult InvokeApplicativeFunction(
        ThreadContext* cx,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::SyntaxTreeFragment*> args);

// Property lookup helpers.
class PropertyLookupResult
{
  public:
    enum class Outcome {
        Error,
        NotFound,
        Found
    };

  private:
    Outcome outcome_;
    StackField<VM::LookupState*> lookupState_;
    StackField<VM::PropertyDescriptor> descriptor_;

    PropertyLookupResult(Outcome outcome,
                         VM::LookupState* lookupState,
                         VM::PropertyDescriptor const& descriptor)
      : outcome_(outcome),
        lookupState_(lookupState),
        descriptor_(descriptor)
    {}

  public:
    PropertyLookupResult(ErrorT_ const& error)
      : PropertyLookupResult(Outcome::Error, nullptr,
                             VM::PropertyDescriptor())
    {}

    static PropertyLookupResult Error() {
        return PropertyLookupResult(Outcome::Error, nullptr,
                                    VM::PropertyDescriptor());
    }
    static PropertyLookupResult NotFound() {
        return PropertyLookupResult(Outcome::NotFound, nullptr,
                                    VM::PropertyDescriptor());
    }
    static PropertyLookupResult Found(
            VM::LookupState* lookupState,
            VM::PropertyDescriptor const& descriptor)
    {
        WH_ASSERT(lookupState != nullptr);
        WH_ASSERT(descriptor.isValid());
        return PropertyLookupResult(Outcome::Found, lookupState, descriptor);
    }

    Outcome outcome() const {
        return outcome_;
    }
    bool isError() const {
        return outcome() == Outcome::Error;
    }
    bool isNotFound() const {
        return outcome() == Outcome::NotFound;
    }
    bool isFound() const {
        return outcome() == Outcome::Found;
    }

    VM::LookupState *lookupState() const {
        WH_ASSERT(isFound());
        return lookupState_;
    }
    VM::PropertyDescriptor const& descriptor() const {
        WH_ASSERT(isFound());
        return descriptor_;
    }
};

PropertyLookupResult GetValueProperty(ThreadContext* cx,
                                      Handle<VM::ValBox> value,
                                      Handle<VM::String*> name);

PropertyLookupResult GetObjectProperty(ThreadContext* cx,
                                       Handle<VM::Wobject*> object,
                                       Handle<VM::String*> name);


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__HEAP_INTERPRETER_HPP

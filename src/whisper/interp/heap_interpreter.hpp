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

VM::EvalResult HeapInterpretSourceFile(ThreadContext* cx,
                                       Handle<VM::SourceFile*> file,
                                       Handle<VM::ScopeObject*> scope);

VM::EvalResult HeapInterpretSourceFile(ThreadContext* cx,
                                       Handle<VM::Frame*> frame,
                                       Handle<VM::SourceFile*> file,
                                       Handle<VM::ScopeObject*> scope);

VM::EvalResult HeapInterpretLoop(ThreadContext* cx,
                                 Handle<VM::Frame*> frame);

Result<VM::Frame*> CreateInitialSyntaxFrame(
        ThreadContext* cx,
        Handle<VM::Frame*> parent,
        Handle<VM::EntryFrame*> entryFrame);

Maybe<VM::FunctionObject*> FunctionObjectForValue(ThreadContext* cx,
                                                  Handle<VM::ValBox> value);

VM::CallResult InvokeOperativeValue(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        ArrayHandle<VM::SyntaxTreeFragment*> args);

VM::CallResult InvokeOperativeFunction(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::SyntaxTreeFragment*> args);

VM::CallResult InvokeApplicativeValue(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        ArrayHandle<VM::ValBox> args);

VM::CallResult InvokeScriptedApplicativeFunction(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        Handle<VM::ScriptedFunction*> calleeScript,
        ArrayHandle<VM::ValBox> args);

VM::CallResult InvokeApplicativeFunction(
        ThreadContext* cx,
        Handle<VM::Frame*> frame,
        Handle<VM::ScopeObject*> callerScope,
        Handle<VM::ValBox> callee,
        Handle<VM::FunctionObject*> calleeFunc,
        ArrayHandle<VM::ValBox> args);

// Property lookup helpers.
class PropertyLookupResult
{
  friend class TraceTraits<PropertyLookupResult>;
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
    static PropertyLookupResult NotFound(VM::LookupState* lookupState) {
        return PropertyLookupResult(Outcome::NotFound, lookupState,
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

    VM::EvalResult toEvalResult(ThreadContext* cx,
                                Handle<VM::Frame*> frame) const;
};

PropertyLookupResult GetValueProperty(ThreadContext* cx,
                                      Handle<VM::ValBox> value,
                                      Handle<VM::String*> name);

PropertyLookupResult GetObjectProperty(ThreadContext* cx,
                                       Handle<VM::Wobject*> object,
                                       Handle<VM::String*> name);

OkResult DefValueProperty(ThreadContext* cx,
                          Handle<VM::ValBox> value,
                          Handle<VM::String*> name,
                          Handle<VM::PropertyDescriptor> descr);

OkResult DefObjectProperty(ThreadContext* cx,
                           Handle<VM::Wobject*> object,
                           Handle<VM::String*> name,
                           Handle<VM::PropertyDescriptor> descr);


} // namespace Interp


//
// GC Specializations
//

template <>
struct TraceTraits<Interp::PropertyLookupResult>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, Interp::PropertyLookupResult const& obj,
                     void const* start, void const* end)
    {
        obj.lookupState_.scan(scanner, start, end);
        obj.descriptor_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, Interp::PropertyLookupResult& obj,
                       void const* start, void const* end)
    {
        obj.lookupState_.update(updater, start, end);
        obj.descriptor_.update(updater, start, end);
    }
};

} // namespace Whisper

#endif // WHISPER__INTERP__HEAP_INTERPRETER_HPP

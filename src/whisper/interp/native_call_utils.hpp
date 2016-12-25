#ifndef WHISPER__INTERP__NATIVE_CALL_UTILS_HPP
#define WHISPER__INTERP__NATIVE_CALL_UTILS_HPP

#include "gc/local.hpp"
#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/control_flow.hpp"
#include "vm/scope_object.hpp"
#include "vm/function.hpp"
#include "vm/frame.hpp"

namespace Whisper {
namespace Interp {


// Helper class to re-enter the interpreter evaluator from a native
// call.
class NativeCallEval
{
  private:
    ThreadContext* cx_;

    // The call info of the original native call.
    StackField<VM::NativeCallInfo> callInfo_;

    // The scope object to perform the evaluation with.
    StackField<VM::ScopeObject*> evalScope_;

    // Syntax tree fragment to evaluate.
    StackField<VM::SyntaxNode*> syntaxNode_;

    // Native function to call when complete.
    VM::NativeCallResumeFuncPtr resumeFunc_;

    // Captured state to pass back to completed call.
    StackField<HeapThing*> resumeState_;

  public:
    NativeCallEval(ThreadContext* cx,
                   VM::NativeCallInfo const& callInfo,
                   VM::ScopeObject* evalScope,
                   VM::SyntaxNode* syntaxNode,
                   VM::NativeCallResumeFuncPtr resumeFunc,
                   HeapThing* resumeState)
      : cx_(cx),
        callInfo_(callInfo),
        evalScope_(evalScope),
        syntaxNode_(syntaxNode),
        resumeFunc_(resumeFunc),
        resumeState_(resumeState)
    {}

    NativeCallEval(ThreadContext* cx,
                   VM::NativeCallInfo const& callInfo,
                   VM::SyntaxNode* syntaxNode,
                   VM::NativeCallResumeFuncPtr resumeFunc,
                   HeapThing* resumeState)
      : cx_(cx),
        callInfo_(callInfo),
        evalScope_(callInfo.callerScope()),
        syntaxNode_(syntaxNode),
        resumeFunc_(resumeFunc),
        resumeState_(resumeState)
    {}

    operator VM::CallResult() const;
};


} // namespace Interp
} // namespace Whisper

#endif // WHISPER__INTERP__NATIVE_CALL_UTILS_HPP

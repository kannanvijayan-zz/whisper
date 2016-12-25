
#include "interp/native_call_utils.hpp"

namespace Whisper {
namespace Interp {


VM::CallResult
RaiseInternalException(ThreadContext* cx,
                       Handle<VM::Frame*> frame,
                       char const* message)
{
    Local<VM::Exception*> exc(cx);
    if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@File called with wrong number of arguments")))
    {
        return ErrorVal();
    }
    return VM::CallResult::Exc(frame, exc);
}

VM::CallResult
RaiseInternalException(ThreadContext* cx,
                       Handle<VM::Frame*> frame,
                       char const* message,
                       Handle<HeapThing*> obj)
{
    Local<VM::Exception*> exc(cx);
    if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@File called with wrong number of arguments", obj)))
    {
        return ErrorVal();
    }
    return VM::CallResult::Exc(frame, exc);
}

NativeCallEval::operator VM::CallResult() const
{
    // Create a new NativeCallResumeFrame
    Local<VM::NativeCallResumeFrame*> resumeFrame(cx_);
    if (!resumeFrame.setResult(VM::NativeCallResumeFrame::Create(
            cx_->inHatchery(),
            callInfo_->frame(),
            callInfo_,
            evalScope_,
            syntaxNode_,
            resumeFunc_,
            resumeState_)))
    {
        return VM::CallResult::Error();
    }

    // Evaluate this frame.
    return VM::CallResult::Continue(resumeFrame);
}


} // namespace Interp
} // namespace Whisper

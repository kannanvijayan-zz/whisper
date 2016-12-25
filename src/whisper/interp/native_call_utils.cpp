
#include "interp/native_call_utils.hpp"

namespace Whisper {
namespace Interp {

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

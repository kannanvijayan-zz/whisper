
#include "interp/native_call_utils.hpp"

namespace Whisper {
namespace Interp {


NativeCallEval::operator VM::CallResult() const
{
    return cx_->setInternalError(
        "NativeCallEval::operator CallResult not implemented.");
}


} // namespace Interp
} // namespace Whisper

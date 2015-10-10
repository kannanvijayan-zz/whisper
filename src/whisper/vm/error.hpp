#ifndef WHISPER__VM__ERROR_HPP
#define WHISPER__VM__ERROR_HPP


#include "vm/core.hpp"
#include "vm/predeclare.hpp"

namespace Whisper {
namespace VM {


//
// Object that represents a run-time error.
//
class Error
{
  public:
    Error() {}
    static Result<Error*> Create(AllocationContext acx);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::Error>
  : public UntracedTraceTraits<VM::Error>
{};


} // namespace Whisper


#endif // WHISPER__VM__ERROR_HPP

#ifndef WHISPER__VM__SYSTEM_HPP
#define WHISPER__VM__SYSTEM_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"

namespace Whisper {
namespace VM {

class Module;

//
// A System object encapsulates all the global state for a single
// execution domain (a compartmentalized object graph and runtime state).
//
class System
{
    friend class GC::TraceTraits<System>;

  public:
    typedef Array<Module *> ModuleArray;

  private:
    HeapField<ModuleArray *> modules_;

  public:
    System()
      : modules_(nullptr)
    {}
};


} // namespace VM
} // namespace Whisper


#include "vm/system_specializations.hpp"


namespace Whisper {
namespace VM {



} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__SYSTEM_HPP

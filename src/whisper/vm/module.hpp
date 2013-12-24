#ifndef WHISPER__VM__MODULE_HPP
#define WHISPER__VM__MODULE_HPP


#include "common.hpp"
#include "debug.hpp"
#include "runtime.hpp"

namespace Whisper {
namespace VM {


//
// Runtime representation of a module.
//
class Module
{
  private:
    Runtime *runtime_;

  public:
    inline Module(Runtime *runtime)
      : runtime_(runtime)
    {
        WH_ASSERT(runtime);
    }

    inline Runtime *runtime() const {
        return runtime_;
    }
};


} //namespace VM
} // namespace Whisper


#endif // WHISPER__VM__MODULE_HPP

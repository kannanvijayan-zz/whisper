#ifndef WHISPER__VM__SYSTEM_HPP
#define WHISPER__VM__SYSTEM_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/vector.hpp"
#include "runtime.hpp"
#include "runtime_inlines.hpp"

namespace Whisper {
namespace VM {

class Module;

//
// A System object encapsulates all the global state for a single
// execution domain (a compartmentalized object graph and runtime state).
//
class System
{
    friend struct GC::TraceTraits<System>;
    friend class Whisper::AllocationContext;

  public:
    typedef Vector<Module *> ModuleVector;

  private:
    HeapField<ModuleVector *> modules_;

    System(ModuleVector *modules)
      : modules_(modules)
    {}

  public:
    static inline System *Create(AllocationContext &acx);
};


} // namespace VM
} // namespace Whisper


#include "vm/system_specializations.hpp"


namespace Whisper {
namespace VM {


/* static */ inline
System *
System::Create(AllocationContext &acx)
{
    constexpr uint32_t ModuleVectorStartCapacity = 20;
    Local<ModuleVector *> modVec(acx,
        ModuleVector::Create(acx, ModuleVectorStartCapacity));
    if (!modVec)
        return nullptr;

    Local<System *> system(acx, acx.create<System>(modVec));
    if (!system)
        return nullptr;

    return system;
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__SYSTEM_HPP

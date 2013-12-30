#ifndef WHISPER__VM__MODULE_HPP
#define WHISPER__VM__MODULE_HPP


#include "common.hpp"
#include "debug.hpp"
#include "slab.hpp"
#include "runtime.hpp"
#include "vm/vector.hpp"

namespace Whisper {

namespace VM {
    class Module;
}

// Specialize Module for SlabThingTraits
template <>
struct SlabThingTraits<VM::Module>
{
    SlabThingTraits() = delete;
    SlabThingTraits(const SlabThingTraits &other) = delete;

    static constexpr bool SPECIALIZED = true;
};

// Specialize Module for AllocationTraits
template <>
struct AllocationTraits<VM::Module>
{
    static constexpr SlabAllocType ALLOC_TYPE = SlabAllocType::Module;
    static constexpr bool TRACED = true;
};


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


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__MODULE_HPP

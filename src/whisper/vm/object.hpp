#ifndef WHISPER__VM__OBJECT_HPP
#define WHISPER__VM__OBJECT_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "vm/property_map_thing.hpp"

namespace Whisper {
namespace VM {


//
// Object is a helper class that can be used to reference actual object
// classes, such as NativeObject, ProxyObject, ForeignObject, etc.
//
class Object : public HeapThing
{
  public:
    Object();
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__OBJECT_HPP

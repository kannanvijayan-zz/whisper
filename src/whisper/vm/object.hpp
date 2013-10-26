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
// It is not a base class for any concrete object type, to allow for
// concrete classes to inherit structure from base classes that are
// shared with non-object heap things.
//
class Object : public HeapThing
{
  public:
    // Not constructible.
    Object(const Object &other) = delete;
};


//
// A HashObject is a simple native object which stores its property
// mappings as a hash table.
//
class HashObject : public HeapThing
{
  private:
    Heap<Object *> prototype_;
    Heap<Tuple *> mappings_;

  public:
    HashObject(Handle<Object *> prototype, Handle<Tuple *> mappings);

    Handle<Object *> prototype() const;
    Handle<Tuple *> mappings() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__OBJECT_HPP

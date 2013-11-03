#ifndef WHISPER__VM__OBJECT_HPP
#define WHISPER__VM__OBJECT_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"
#include "tuple.hpp"

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
    uint32_t entries_;
    Heap<Tuple *> mappings_;

    static constexpr uint32_t INITIAL_ENTRIES = 4;

  public:
    HashObject(Handle<Object *> prototype);
    bool initialize(RunContext *cx);

    Handle<Object *> prototype() const;
    Handle<Tuple *> mappings() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__OBJECT_HPP

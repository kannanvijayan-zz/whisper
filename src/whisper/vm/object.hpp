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
// A HashObject is a simple native object format which stores its property
// mappings as a hash table.
//
class HashObject : public HeapThing,
                   public TypedHeapThing<HeapType::HashObject>
{
  private:
    Heap<Object *> prototype_;
    Heap<Tuple *> mappings_;
    uint32_t entries_;

    static constexpr uint32_t INITIAL_ENTRIES = 4;
    static constexpr float MAX_FILL_RATIO = 0.75;

  public:
    HashObject(Handle<Object *> prototype);
    bool initialize(RunContext *cx);

    Handle<Object *> prototype() const;
    Handle<Tuple *> mappings() const;

    uint32_t propertyCapacity() const;
    uint32_t numProperties() const;

    void setPrototype(Handle<Object *> newProto);

    bool defineValueProperty(RunContext *cx,
                             Handle<Value> key,
                             Handle<Value> val);

  private:
    uint32_t lookupOwnProperty(RunContext *cx, Handle<Value> keyString,
                               bool forAdd=false);

    uint32_t hashValue(RunContext *cx, const Value &key) const;

    static uint32_t KeySlotOffset(uint32_t entry);
    static uint32_t ValueSlotOffset(uint32_t entry);

    Handle<Value> getEntryKey(uint32_t entry) const;
    Handle<Value> getEntryValue(uint32_t entry) const;
    void setEntryKey(uint32_t entry, const Value &key);
    void setEntryValue(uint32_t entry, const Value &val);

    bool enlarge(RunContext *cx);
};


//
// A HashObjectValueProperty defines a value property binding.
//
class HashObjectValueProperty
  : public HeapThing,
    public TypedHeapThing<HeapType::HashObjectValueProperty>
{
  private:
    static constexpr uint32_t ConfigurableFlag = 0x1;
    static constexpr uint32_t EnumerableFlag = 0x2;
    static constexpr uint32_t WritableFlag = 0x4;

    Heap<Value> value_;

  public:
    struct Config {
        bool configurable;
        bool enumerable;
        bool writable;

        Config();
        Config(bool c, bool e, bool w);
    };

  public:
    HashObjectValueProperty(const Config &config, const Value &val);

    bool isConfigurable() const;
    bool isEnumerable() const;
    bool isWritable() const;

    const Heap<Value> &value() const;
    Heap<Value> &value();

    void setValue(const Value &val);

  private:
    void initialize(const Config &conf);
};


//
// A HashObjectValueProperty defines a value property binding.
//
class HashObjectAccessorProperty
  : public HeapThing,
    public TypedHeapThing<HeapType::HashObjectAccessorProperty>
{
  private:
    static constexpr uint32_t ConfigurableFlag = 0x1;
    static constexpr uint32_t EnumerableFlag = 0x2;

    Heap<Value> getter_;
    Heap<Value> setter_;

  public:
    struct Config {
        bool configurable;
        bool enumerable;

        Config();
        Config(bool c, bool e);
    };

  public:
    HashObjectAccessorProperty(const Config &config,
                               const Value &getter,
                               const Value &setter);

    bool isConfigurable() const;
    bool isEnumerable() const;

    const Heap<Value> &getter() const;
    Heap<Value> &getter();

    const Heap<Value> &setter() const;
    Heap<Value> &setter();

    void setGetter(const Value &val);
    void setSetter(const Value &val);

  private:
    void initialize(const Config &conf);
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__OBJECT_HPP

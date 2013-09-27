#ifndef WHISPER__VM__PROPERTY_DESCRIPTOR_HPP
#define WHISPER__VM__PROPERTY_DESCRIPTOR_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"

namespace Whisper {
namespace VM {

//
// A PropertyDescriptor object, as per ECMA-262 specification 8.2.5
//
class PropertyDescriptor
{
  public:
    enum Flags : uint32_t {
        HasValue            = (1 << 0),
        HasGetter           = (1 << 1),
        HasSetter           = (1 << 2),
        IsConfigurable      = (1 << 3),
        IsEnumerable        = (1 << 4),
        IsWritable          = (1 << 5)
    };

  private:
    union {
        Value value_;
        struct {
            Value getter_;
            Value setter_;
        } accessor_;
    };
    uint32_t flags_;

  public:

    PropertyDescriptor(Value val, Value get, Value set,
                       bool hasValue, bool hasGetter, bool hasSetter,
                       bool isConfig, bool isEnum, bool isWrite);

    bool hasValue() const;
    const Value &value() const;

    bool hasGetter() const;
    const Value &getter() const;

    bool hasSetter() const;
    const Value &setter() const;

    bool isAccessor() const;
    bool isData() const;
    bool isGeneric() const;

    bool isConfigurable() const;
    bool isEnumerable() const;
    bool isWritable() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__PROPERTY_DESCRIPTOR_HPP

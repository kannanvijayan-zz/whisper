#ifndef WHISPER__VM__PROPERTY_DESCRIPTOR_HPP
#define WHISPER__VM__PROPERTY_DESCRIPTOR_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "value.hpp"

namespace Whisper {
namespace VM {

//
// A PropertyDescriptor object, as per ECMA-262 specification 8.2.5
//
class PropertyDescriptor : public HeapThing<HeapType::PropertyDescriptor>
{
  public:
    static constexpr uint32_t HasValueShift = 0;
    static constexpr uint32_t HasGetterShift = 1;
    static constexpr uint32_t HasSetterShift = 2;
    static constexpr uint32_t IsConfigurableShift = 3;
    static constexpr uint32_t IsEnumerableShift = 4;
    static constexpr uint32_t IsWritableShift = 5;

  private:
    Value valueOrGetter_;
    Value setter_;

    void initDataFlags(bool isConfigurable, bool isEnumerable,
                       bool isWritable)
    {
        uint32_t flags = 0;
        flags |= static_cast<uint32_t>(1) << HasValueShift;
        flags |= static_cast<uint32_t>(isConfigurable) << IsConfigurableShift;
        flags |= static_cast<uint32_t>(isEnumerable) << IsEnumerableShift;
        flags |= static_cast<uint32_t>(isWritable) << IsWritableShift;
        initFlags(flags);
    }

    void initAccessorFlags(bool hasGetter, bool hasSetter,
                           bool isConfigurable, bool isEnumerable)
    {
        WH_ASSERT(hasGetter || hasSetter);
        uint32_t flags = 0;
        flags |= static_cast<uint32_t>(hasGetter) << HasGetterShift;
        flags |= static_cast<uint32_t>(hasSetter) << HasSetterShift;
        flags |= static_cast<uint32_t>(isConfigurable) << IsConfigurableShift;
        flags |= static_cast<uint32_t>(isEnumerable) << IsEnumerableShift;
        initFlags(flags);
    }

  public:
    PropertyDescriptor(Value value, bool isConfig, bool isEnum, bool isWrite)
      : valueOrGetter_(value), setter_(UndefinedValue())
    {
        initDataFlags(isConfig, isEnum, isWrite);
    }

    PropertyDescriptor(bool isGet, Value getOrSet, bool isConfig, bool isEnum)
      : valueOrGetter_(isGet ? getOrSet : UndefinedValue()),
        setter_(isGet ? UndefinedValue() : getOrSet)
    {
        initAccessorFlags(isGet, !isGet, isConfig, isEnum);
    }

    PropertyDescriptor(Value get, Value set, bool isConfig, bool isEnum)
      : valueOrGetter_(get), setter_(set)
    {
        initAccessorFlags(true, true, isConfig, isEnum);
    }

    bool hasValue() const {
        return flags() & (static_cast<uint32_t>(1) << HasValueShift);
    }

    bool hasGetter() const {
        return flags() & (static_cast<uint32_t>(1) << HasGetterShift);
    }

    bool hasSetter() const {
        return flags() & (static_cast<uint32_t>(1) << HasSetterShift);
    }

    bool isAccessor() const {
        WH_ASSERT_IF(hasValue(), !hasGetter() && !hasSetter());
        WH_ASSERT_IF(hasGetter() || hasSetter(), !hasValue());
        return hasGetter() || hasSetter();
    }

    bool isData() const {
        WH_ASSERT_IF(hasValue(), !hasGetter() && !hasSetter());
        WH_ASSERT_IF(hasGetter() || hasSetter(), !hasValue());
        return hasValue();
    }

    bool isGeneric() const {
        return !isAccessor() && !isData();
    }

    bool isConfigurable() const {
        return flags() & (static_cast<uint32_t>(1) << IsConfigurableShift);
    }

    bool isEnumerable() const {
        return flags() & (static_cast<uint32_t>(1) << IsEnumerableShift);
    }

    bool isWritable() const {
        WH_ASSERT(isData());
        return flags() & (static_cast<uint32_t>(1) << IsWritableShift);
    }
};

typedef HeapThingWrapper<PropertyDescriptor> WrappedPropertyDescriptor;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__PROPERTY_DESCRIPTOR_HPP

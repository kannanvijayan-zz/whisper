
#include "property_descriptor.hpp"

namespace Whisper {
namespace VM {


PropertyDescriptor::PropertyDescriptor(
            Value val, Value get, Value set,
            bool hasValue, bool hasGetter, bool hasSetter,
            bool isConfig, bool isEnum, bool isWrite)
  : flags_(0)
{
    WH_ASSERT_IF(hasValue, !hasGetter && !hasSetter);

    WH_ASSERT_IF(!hasValue, !isWrite);
    WH_ASSERT_IF(!hasValue, val.isUndefined());
    WH_ASSERT_IF(!hasGetter, get.isUndefined());
    WH_ASSERT_IF(!hasSetter, set.isUndefined());
    WH_ASSERT_IF(hasGetter, !get.isUndefined());
    WH_ASSERT_IF(hasSetter, !set.isUndefined());

    value_ = val;
    accessor_.getter_ = get;
    accessor_.setter_ = set;
    flags_ = (hasValue  ? static_cast<uint32_t>(HasValue)       : 0)
           | (hasGetter ? static_cast<uint32_t>(HasGetter)      : 0)
           | (hasSetter ? static_cast<uint32_t>(HasSetter)      : 0)
           | (isConfig  ? static_cast<uint32_t>(IsConfigurable) : 0)
           | (isEnum    ? static_cast<uint32_t>(IsEnumerable)   : 0)
           | (isWrite   ? static_cast<uint32_t>(IsWritable)     : 0);
}

bool
PropertyDescriptor::hasValue() const
{
    return flags_ & HasValue;
}

const Value &
PropertyDescriptor::value() const
{
    WH_ASSERT(hasValue());
    return value_;
}

bool
PropertyDescriptor::hasGetter() const
{
    return flags_ & HasGetter;
}

const Value &
PropertyDescriptor::getter() const
{
    WH_ASSERT(hasGetter());
    return accessor_.getter_;
}

bool
PropertyDescriptor::hasSetter() const
{
    return flags_ & HasSetter;
}

const Value &
PropertyDescriptor::setter() const
{
    WH_ASSERT(hasSetter());
    return accessor_.setter_;
}

bool
PropertyDescriptor::isAccessor() const
{
    return hasGetter() || hasSetter();
}

bool
PropertyDescriptor::isData() const
{
    return hasValue();
}

bool
PropertyDescriptor::isGeneric() const
{
    return !isAccessor() && !isData();
}

bool
PropertyDescriptor::isConfigurable() const
{
    return flags_ & IsConfigurable;
}

bool
PropertyDescriptor::isEnumerable() const
{
    return flags_ & IsEnumerable;
}

bool
PropertyDescriptor::isWritable() const
{
    return flags_ & IsWritable;
}


} // namespace VM
} // namespace Whisper

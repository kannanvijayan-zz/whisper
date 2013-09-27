
#include "reference.hpp"

namespace Whisper {
namespace VM {


/*static*/ bool
Reference::IsValidBase(const Value &val)
{
    WH_ASSERT(val.isValid());
    if (val.isObject() || val.isBoolean() || val.isString())
        return true;
    if (val.isNumber() || val.isUndefined())
        return true;
    // TODO: check for environment record.
    return false;
}

/*static*/ bool
Reference::IsValidName(const Value &val)
{
    WH_ASSERT(val.isValid());
    return val.isString() || val.isInt32();
}

/*static*/ bool
Reference::IsValidThis(const Value &val)
{
    WH_ASSERT(val.isValid());
    return val.isObject() || val.isBoolean() || val.isString() ||
           val.isNumber();
}

Reference::Reference(const Value &base, const Value &name, const Value &thisv,
                     bool strict)
  : base_(base), name_(name), thisv_(thisv), isStrict_(strict), isSuper_(true)
{
    WH_ASSERT(IsValidName(name));
    WH_ASSERT(IsValidBase(base));
    WH_ASSERT(IsValidThis(thisv));
}

Reference::Reference(const Value &base, const Value &name, const Value &thisv)
  : Reference(base, name, thisv, false)
{
    WH_ASSERT(IsValidName(name));
    WH_ASSERT(IsValidBase(base));
    WH_ASSERT(IsValidThis(thisv));
}

Reference::Reference(const Value &base, const Value &name, bool strict)
  : base_(base), name_(name), thisv_(UndefinedValue()),
    isStrict_(strict), isSuper_(false)
{
    WH_ASSERT(IsValidName(name));
    WH_ASSERT(IsValidBase(base));
}

Reference::Reference(const Value &base, const Value &name)
  : Reference(base, name, false)
{
    WH_ASSERT(IsValidName(name));
    WH_ASSERT(IsValidBase(base));
}

const Value &
Reference::base() const
{
    return base_;
}

const Value &
Reference::name() const
{
    return name_;
}

bool
Reference::isStrict() const
{
    return isStrict_;
}

bool
Reference::isSuper() const
{
    return isSuper_;
}

const Value &
Reference::thisv() const
{
    WH_ASSERT(isSuper());
    return thisv_;
}

bool
Reference::hasPrimitiveBase() const
{
    return base_.isBoolean() || base_.isString() || base_.isNumber();
}

bool
Reference::isProperty() const
{
    return base_.isObject() || hasPrimitiveBase();
}

bool
Reference::isUnresolvable() const
{
    return base_.isUndefined();
}


} // namespace VM
} // namespace Whisper

#ifndef WHISPER__VM__REFERENCE_HPP
#define WHISPER__VM__REFERENCE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "value.hpp"

namespace Whisper {
namespace VM {

//
// A Reference object, as per ECMA-262 specification 8.2.4
//
class Reference : public HeapThingPayload<HeapType::Reference>
{
  public:
    static constexpr uint32_t IsStrictShift = 0;
    static constexpr uint32_t IsSuperShift = 1;

  private:
    Value base_;
    Value name_;
    Value thisv_;

    void initReferenceFlags(bool isStrict, bool isSuper) {
        uint32_t flags = 0;
        flags |= static_cast<uint32_t>(isStrict) << IsStrictShift;
        flags |= static_cast<uint32_t>(isSuper) << IsSuperShift;
        initFlags(flags);
    }

  public:
    // A reference name has to be either a string, or an int32.
    inline static bool IsValidBase(const Value &val) {
        WH_ASSERT(val.isValid());
        if (val.isObject() || val.isBoolean() || val.isString())
            return true;
        if (val.isNumber() || val.isUndefined())
            return true;
        // TODO: check for environment record.
        return false;
    }
    inline static bool IsValidName(const Value &val) {
        WH_ASSERT(val.isValid());
        return val.isString() || val.isInt32();
    }
    inline static bool IsValidThis(const Value &val) {
        WH_ASSERT(val.isValid());
        return val.isObject() || val.isBoolean() || val.isString() ||
               val.isNumber();
    }

    Reference(const Value &base, const Value &name)
      : base_(base), name_(name), thisv_(UndefinedValue())
    {
        WH_ASSERT(IsValidName(name));
        WH_ASSERT(IsValidBase(base));
        initReferenceFlags(false, false);
    }

    Reference(const Value &base, const Value &name, bool strict)
      : base_(base), name_(name), thisv_(UndefinedValue())
    {
        WH_ASSERT(IsValidName(name));
        WH_ASSERT(IsValidBase(base));
        initReferenceFlags(strict, false);
    }

    Reference(const Value &base, const Value &name, const Value &thisv)
      : base_(base), name_(name), thisv_(thisv)
    {
        WH_ASSERT(IsValidName(name));
        WH_ASSERT(IsValidBase(base));
        WH_ASSERT(IsValidThis(thisv));
        initReferenceFlags(false, true);
    }

    Reference(const Value &base, const Value &name, bool strict,
              const Value &thisv)
      : base_(base), name_(name), thisv_(thisv)
    {
        WH_ASSERT(IsValidName(name));
        WH_ASSERT(IsValidBase(base));
        WH_ASSERT(IsValidThis(thisv));
        initReferenceFlags(strict, true);
    }

    const Value &base() const {
        return base_;
    }

    const Value &name() const {
        return name_;
    }

    bool isStrict() const {
        return flags() & (static_cast<uint32_t>(1) << IsStrictShift);
    }

    bool isSuper() const {
        return flags() & (static_cast<uint32_t>(1) << IsSuperShift);
    }

    const Value &thisv() const {
        WH_ASSERT(isSuper());
        return thisv_;
    }

    bool hasPrimitiveBase() const {
        return base_.isBoolean() || base_.isString() || base_.isNumber();
    }

    bool isProperty() const {
        return base_.isObject() || hasPrimitiveBase();
    }

    bool isUnresolvable() const {
        return base_.isUndefined();
    }
};

typedef HeapThingWrapper<Reference, HeapType::Reference> WrappedReference;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__REFERENCE_HPP

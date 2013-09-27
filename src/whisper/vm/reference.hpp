#ifndef WHISPER__VM__REFERENCE_HPP
#define WHISPER__VM__REFERENCE_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"

namespace Whisper {
namespace VM {

//
// A Reference object, as per ECMA-262 specification 8.2.4
//
class Reference
{
  private:
    Value base_;
    Value name_;
    Value thisv_;
    bool isStrict_;
    bool isSuper_;

  public:
    static bool IsValidBase(const Value &val);
    static bool IsValidName(const Value &val);
    static bool IsValidThis(const Value &val);

    Reference(const Value &base, const Value &name, const Value &thisv,
              bool strict);
    Reference(const Value &base, const Value &name, const Value &thisv);

    Reference(const Value &base, const Value &name, bool strict);
    Reference(const Value &base, const Value &name);

    const Value &base() const;
    const Value &name() const;
    bool isStrict() const;
    bool isSuper() const;
    const Value &thisv() const;
    bool hasPrimitiveBase() const;
    bool isProperty() const;
    bool isUnresolvable() const;
};


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__REFERENCE_HPP

#ifndef WHISPER__VM__VALUE_TYPE_HPP
#define WHISPER__VM__VALUE_TYPE_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace VM {


//
// Primitive type codes define the numeric code of
// a given primitive type.
//
enum class PrimitiveTypeCode : uint32_t
{
    INVALID     = 0,
    Int         = 1
};

inline const char *
PrimitiveTypeCodeString(PrimitiveTypeCode code)
{
    switch(code) {
      case PrimitiveTypeCode::INVALID:
        return "INVALID";

      case PrimitiveTypeCode::Int:
        return "int";
    }

    return "UNKNOWN";
}

inline bool
IsValidPrimitiveTypeCode(PrimitiveTypeCode code)
{
    switch(code) {
      case PrimitiveTypeCode::INVALID:
        return false;

      case PrimitiveTypeCode::Int:
        return true;
    }

    return false;
}


//
// Describes the type of a value.  Primitive types are represented
// as tagged enum values from PrimitiveTypeCode
//
class ValueType
{
  private:
    uintptr_t data_;

    constexpr static unsigned PRIMITIVE_SHIFT = 1;
    constexpr static uintptr_t PRIMITIVE_TAG = 0x1u;

  public:
    inline ValueType(PrimitiveTypeCode code)
      : data_((ToUInt32(code) << PRIMITIVE_SHIFT) | PRIMITIVE_TAG)
    {
        WH_ASSERT(IsValidPrimitiveTypeCode(code));
    }

    inline bool isPrimitive() const {
        return data_ & PRIMITIVE_TAG;
    }

    inline bool isInt() const {
        return isPrimitive() && primitiveTypeCode() == PrimitiveTypeCode::Int;
    }

    inline PrimitiveTypeCode primitiveTypeCode() const {
        WH_ASSERT(isPrimitive());
        return static_cast<PrimitiveTypeCode>(data_ >> PRIMITIVE_SHIFT);
    }
};


} // namespace VM
} // namespace Whisper


#include "vm/value_type_specializations.hpp"


#endif // WHISPER__VM__VALUE_TYPE_HPP

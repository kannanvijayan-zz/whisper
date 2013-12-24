#ifndef WHISPER__VM__TYPE_HPP
#define WHISPER__VM__TYPE_HPP


#include "common.hpp"
#include "debug.hpp"

namespace Whisper {
namespace VM {


//
// Primitive type codes define the numeric code of
// a given primitive type.
//
enum class PrimitiveCode : uint32_t
{
    INVALID     = 0,
    Int         = 1
};

inline const char *
PrimitiveCodeString(PrimitiveCode code)
{
    switch(code) {
      case PrimitiveCode::INVALID:
        return "INVALID";

      case PrimitiveCode::Int:
        return "int";
    }

    return "UNKNOWN";
}

inline bool
IsValidPrimitiveCode(PrimitiveCode code)
{
    switch(code) {
      case PrimitiveCode::INVALID:
        return false;

      case PrimitiveCode::Int:
        return true;
    }

    return false;
}


//
// Models the type of a value.  Primitive types are represented
// directly.
//
class ValueType
{
  private:
    uintptr_t data_;

    constexpr static unsigned PRIMITIVE_SHIFT = 1;
    constexpr static uintptr_t PRIMITIVE_TAG = 0x1u;

  public:
    inline ValueType(PrimitiveCode code)
      : data_((ToUInt32(code) << PRIMITIVE_SHIFT) | PRIMITIVE_TAG)
    {
        WH_ASSERT(IsValidPrimitiveCode(code));
    }

    inline bool isPrimitive() const {
        return data_ & PRIMITIVE_TAG;
    }

    inline PrimitiveCode primitiveCode() const {
        WH_ASSERT(isPrimitive());
        return static_cast<PrimitiveCode>(data_ >> PRIMITIVE_SHIFT);
    }
};


} //namespace VM
} // namespace Whisper


#endif // WHISPER__VM__TYPE_HPP

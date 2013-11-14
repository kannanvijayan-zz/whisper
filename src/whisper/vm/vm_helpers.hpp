#ifndef WHISPER__VM__VM_HELPERS_HPP
#define WHISPER__VM__VM_HELPERS_HPP

#include "value.hpp"

namespace Whisper {
namespace VM {

template <typename CharT>
bool IsNormalizedPropertyId(const CharT *str, uint32_t length);

// Check if a Value is a valid property name.
bool IsNormalizedPropertyId(const Value &val);


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__VM_HELPERS_HPP

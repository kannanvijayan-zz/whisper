#ifndef WHISPER__VM__LEXICAL_NAMESPACE_PRELUDE_HPP
#define WHISPER__VM__LEXICAL_NAMESPACE_PRELUDE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"

//
// Pre-specialize HeapTraits here because
// the parent pointerfor LexicalNamespace requires
// the HeapField wrapper to know about the specialization
// in advance.
//

namespace Whisper {
namespace VM {

class LexicalNamespace;

} // namespace VM
} // namespace Whisper

namespace Whisper {
namespace GC {


template <>
struct HeapTraits<VM::LexicalNamespace>
{
    HeapTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr AllocFormat Format = AllocFormat::LexicalNamespace;

    template <typename... Args>
    static inline uint32_t SizeOf(Args... rest);
};


} // namespace GC
} // namespace Whisper


#endif // WHISPER__VM__LEXICAL_NAMESPACE_PRELUDE_HPP

#ifndef WHISPER__NAME_POOL_HPP
#define WHISPER__NAME_POOL_HPP

#include "vm/array.hpp"
#include "vm/string.hpp"

#define WHISPER_DEFN_NAME_POOL(_) \
    _(AtInteger,    "@integer")

namespace Whisper {
namespace NamePool {


enum class Id : uint32_t {
    INVALID = 0,
#define NAME_POOL_ID_(name, str) name,
    WHISPER_DEFN_NAME_POOL(NAME_POOL_ID_)
#undef NAME_POOL_ID_
    LIMIT
};

constexpr uint32_t IndexOfId(Id id) {
    return static_cast<uint32_t>(id) - 1;
}

constexpr uint32_t Size() {
    return IndexOfId(Id::LIMIT);
}


} // namespace NamePool
} // namespace Whisper


#endif // WHISPER__NAME_POOL_HPP

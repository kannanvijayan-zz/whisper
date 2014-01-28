#ifndef WHISPER__FNV_HASH_HPP
#define WHISPER__FNV_HASH_HPP

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"

namespace Whisper {


template <typename U> struct FNVHashParams {};

template <> struct FNVHashParams<uint32_t> {
    typedef uint32_t HashT;
};
template <> struct FNVHashParams<uint64_t> {
    typedef uint64_t HashT;
    static constexpr HashT PRIME = (HashT(1u) << 40) + (HashT(1u) << 8) + 0xb3;
    static constexpr HashT OFFSET = 1099511628211ull;
};


class FNVHash
{
  private:
    uint32_t hash_;

    static constexpr uint32_t PRIME = (static_cast<uint32_t>(1u) << 24) |
                                      (static_cast<uint32_t>(1u) << 8)  |
                                      0x93;
    static constexpr uint32_t OFFSET = 2166136261UL;

  public:
    inline FNVHash() : hash_(OFFSET) {}

    inline void update(uint8_t octet) {
        hash_ ^= octet;
        hash_ *= PRIME;
    }

    inline uint32_t digest() const {
        return hash_;
    }

    inline uint32_t finish(uint8_t octet) {
        update(octet);
        return digest();
    }
};


} // namespace Whisper

#endif // WHISPER__FNV_HASH_HPP

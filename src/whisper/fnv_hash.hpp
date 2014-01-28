#ifndef WHISPER__FNV_HASH_HPP
#define WHISPER__FNV_HASH_HPP

#include "common.hpp"
#include "debug.hpp"
#include "helpers.hpp"

namespace Whisper {


template <typename U> struct FNVHashParams {};

template <> struct FNVHashParams<uint32_t> {
    typedef uint32_t HashT;
    static constexpr HashT PRIME = (HashT(1u) << 24) + (HashT(1u) << 8) + 0x93;
    static constexpr HashT OFFSET = 2166136261ul;
};
template <> struct FNVHashParams<uint64_t> {
    typedef uint64_t HashT;
    static constexpr HashT PRIME = (HashT(1u) << 40) + (HashT(1u) << 8) + 0xb3;
    static constexpr HashT OFFSET = 1099511628211ull;
};


template <typename T>
class FNVHash
{
  public:
    typedef T HashT;

  private:
    HashT hash_;

  public:
    inline FNVHash() : hash_(FNVHashParams<T>::FNV_OFFSET) {}

    inline void update(uint8_t octet) {
        hash_ ^= octet;
        hash_ *= FNVHashParams<T>::FNV_PRIME;
    }

    inline HashT digest() const {
        return hash_;
    }

    inline HashT finish(uint8_t octet) {
        update(octet);
        return digest();
    }
};


} // namespace Whisper

#endif // WHISPER__FNV_HASH_HPP
